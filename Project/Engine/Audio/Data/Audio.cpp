#include "Audio.h"
#include "AudioPitchStream.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <thread>

namespace {
    constexpr size_t kStreamFrames = 1024;
    constexpr size_t kStreamBuffers = 4;

    /// @brief Windowsのイベントハンドルを解放するデリーター
    struct EventDeleter {
        /// @brief イベントハンドルを解放
        /// @param handle 解放対象のハンドル
        void operator()(void* handle) const {
            if (handle) {
                CloseHandle(handle);
            }
        }
    };
    using UniqueEvent = std::unique_ptr<void, EventDeleter>;
}

/// @brief 元PCMと変換済み音声バッファの寿命を所有する単一再生インスタンス
class ClipInstance final : public IXAudio2VoiceCallback {
public:
    /// @brief 元PCMを共有して再生インスタンスを生成
    /// @param pcm 元PCMの共有バッファ
    /// @param format 元PCMの形式
    /// @param loop ループ再生する場合はtrue
    ClipInstance(std::shared_ptr<const std::vector<BYTE>> pcm, const WAVEFORMATEX& format, bool loop)
        : pcm_(std::move(pcm)), format_(format), loop_(loop) {
    }

    /// @brief 変換スレッドとVoiceを停止して解放
    ~ClipInstance() {
        Stop();
    }

    /// @brief 音量と音程を適用して再生を開始
    /// @param engine XAudio2の再生環境
    /// @param volume 音量、0から1の範囲
    /// @param semitones 元の音程からの半音数
    /// @param isPaused 再生開始時から一時停止する場合はtrue
    /// @return 再生開始に成功した場合はtrue
    bool Play(IXAudio2* engine, float volume, float semitones, bool isPaused) {
        engine_ = engine;
        volume_ = std::clamp(volume, 0.0f, 1.0f);
        isPaused_ = isPaused;
        if (semitones != 0.0f) {
            return StartPitchVoice(semitones, 0);
        }

        // 未変更音源は直接再生して変換スレッドとFFTの負荷を回避
        IXAudio2SourceVoice* raw = nullptr;
        if (FAILED(engine_->CreateSourceVoice(&raw, &format_))) {
            return false;
        }
        voice_.reset(raw);
        voice_->SetVolume(volume_);
        XAUDIO2_BUFFER buffer{};
        buffer.pAudioData = pcm_->data();
        buffer.AudioBytes = static_cast<UINT32>(pcm_->size() - pcm_->size() % format_.nBlockAlign);
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        if (loop_) {
            buffer.LoopLength = buffer.AudioBytes / format_.nBlockAlign;
            buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        }
        if (FAILED(voice_->SubmitSourceBuffer(&buffer))) {
            return false;
        }

        // 一時停止中の新規再生はバッファ投入までに留め、瞬間的な音漏れを防止
        if (isPaused_) {
            return true;
        }
        return SUCCEEDED(voice_->Start());
    }

    /// @brief 再生位置と投入済みバッファを維持して一時停止状態を変更
    /// @param isPaused 一時停止する場合はtrue
    void SetPaused(bool isPaused) {
        if (isPaused_ == isPaused) {
            return;
        }
        isPaused_ = isPaused;
        if (!voice_) {
            return;
        }

        // バッファを破棄せずVoiceだけを停止し、再開時も同じサンプル位置から継続
        if (isPaused_) {
            voice_->Stop();
        } else {
            voice_->Start();
        }
    }

    /// @brief 再生中の音量を変更
    /// @param volume 音量、0から1の範囲
    void SetVolume(float volume) {
        volume_ = std::clamp(volume, 0.0f, 1.0f);
        if (voice_) {
            voice_->SetVolume(volume_);
        }
    }

    /// @brief 現在の時間位置を維持して音程を変更
    /// @param semitones 元の音程からの半音数
    /// @return 音程変更に成功した場合はtrue
    bool SetPitch(float semitones) {
        if (worker_.joinable()) {

            // DSP状態は専用スレッドだけで更新してメインスレッドとの競合を防止
            pitch_.store(semitones);
            return true;
        }
        if (!voice_ || semitones == 0.0f) {
            return true;
        }

        // 直接再生からの移行はSamplesPlayedを元PCMの時間位置として引き継ぎ
        XAUDIO2_VOICE_STATE state{};
        voice_->GetState(&state);
        const size_t totalFrames = pcm_->size() / format_.nBlockAlign;
        size_t startFrame = static_cast<size_t>(state.SamplesPlayed);
        if (loop_) {
            startFrame %= totalFrames;
        } else if (startFrame >= totalFrames) {
            return true;
        }
        return StartPitchVoice(semitones, startFrame);
    }

    /// @brief 再生終了状態を取得
    /// @return 再生が終了した場合はtrue
    bool Finished() const {
        if (!voice_ || failed_.load()) {
            return true;
        }

        // 最終バッファの投入完了を確認した後にキューを調べて終端直前の競合を回避
        const bool allSubmitted = inputEnded_.load();
        XAUDIO2_VOICE_STATE state{};
        voice_->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (worker_.joinable()) {
            return allSubmitted && state.BuffersQueued == 0;
        }
        return state.BuffersQueued == 0;
    }

    /// @brief ループ指定を取得
    /// @return ループ再生の場合はtrue
    bool IsLooping() const { return loop_; }

    /// @brief 変換と再生を停止して非同期参照を解消
    void Stop() {
        if (worker_.joinable()) {

            // 待機を解除して変換処理を完了させた後にVoiceとPCM参照を解放
            worker_.request_stop();
            SetEvent(event_.get());
            worker_.join();
        }

        // DestroyVoiceによるコールバック完了待ちより前にイベントや出力領域を破棄しない制約
        voice_.reset();
        stream_.reset();
        event_.reset();
    }

private:
    struct BufferSlot {
        std::vector<float> samples;
        std::atomic<bool> inFlight{ false };
    };

    /// @brief 指定時間位置からFloat32の変換再生へ移行
    /// @param semitones 元の音程からの半音数
    /// @param startFrame 元PCM内の開始フレーム番号
    /// @return 移行に成功した場合はtrue
    bool StartPitchVoice(float semitones, size_t startFrame) {
        auto stream = std::make_unique<AudioPitchStream>();
        if (!stream->Initialize(pcm_, format_, semitones, loop_, startFrame)) {
            return false;
        }
        UniqueEvent event(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        if (!event) {
            return false;
        }

        // 時間倍率を変更せず、元PCMと同じSample RateのFloat32音声として再生
        WAVEFORMATEX outputFormat{};
        outputFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        outputFormat.nChannels = format_.nChannels;
        outputFormat.nSamplesPerSec = format_.nSamplesPerSec;
        outputFormat.wBitsPerSample = 32;
        outputFormat.nBlockAlign = static_cast<WORD>(outputFormat.nChannels * sizeof(float));
        outputFormat.nAvgBytesPerSec = outputFormat.nSamplesPerSec * outputFormat.nBlockAlign;
        IXAudio2SourceVoice* raw = nullptr;
        if (FAILED(engine_->CreateSourceVoice(&raw, &outputFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this))) {
            return false;
        }
        UniqueSourceVoice nextVoice(raw);
        nextVoice->SetVolume(volume_);
        stream_ = std::move(stream);
        event_ = std::move(event);
        pitch_.store(semitones);
        inputEnded_.store(false);
        generatedFrames_ = 0;
        remainingFrames_ = pcm_->size() / format_.nBlockAlign - startFrame;
        for (auto& slot : slots_) {
            slot.samples.resize(kStreamFrames * format_.nChannels);
            slot.inFlight.store(false);
            if (!FillBuffer(nextVoice.get(), slot)) {
                return false;
            }
        }

        // 先読みと最初のバッファ投入が成功するまで以前の直接再生を保持
        voice_.reset();
        voice_ = std::move(nextVoice);
        if (!isPaused_ && FAILED(voice_->Start())) {
            return false;
        }
        worker_ = std::jthread([this](std::stop_token stop) { StreamAudio(stop); });
        return true;
    }

    /// @brief XAudio2へ再利用可能な変換済みバッファを投入
    /// @param voice 投入先のSourceVoice
    /// @param slot 再利用可能な出力バッファ
    /// @return 投入または終端処理に成功した場合はtrue
    bool FillBuffer(IXAudio2SourceVoice* voice, BufferSlot& slot) {
        if (inputEnded_.load()) {
            return true;
        }
        stream_->SetPitch(pitch_.load());
        const size_t frames = stream_->Read(slot.samples.data(), kStreamFrames);
        if (frames == 0) {
            inputEnded_.store(true);
            return true;
        }
        generatedFrames_ += frames;
        const bool isLast = !loop_ && generatedFrames_ == remainingFrames_;
        XAUDIO2_BUFFER buffer{};
        buffer.pAudioData = reinterpret_cast<const BYTE*>(slot.samples.data());
        buffer.AudioBytes = static_cast<UINT32>(frames * format_.nChannels * sizeof(float));
        buffer.pContext = &slot;
        if (isLast) {

            // 元PCMと同じ総フレーム数で終端を通知して短いSEの末尾も保持
            buffer.Flags = XAUDIO2_END_OF_STREAM;
        }
        slot.inFlight.store(true);
        if (FAILED(voice->SubmitSourceBuffer(&buffer))) {
            slot.inFlight.store(false);
            return false;
        }
        if (isLast) {
            inputEnded_.store(true);
        }
        return true;
    }

    /// @brief バッファ消費通知に応じて音程変換と補充を継続
    /// @param stop 停止要求を受け取るトークン
    void StreamAudio(std::stop_token stop) {
        while (!stop.stop_requested() && !inputEnded_.load() && !failed_.load()) {
            if (WaitForSingleObject(event_.get(), INFINITE) != WAIT_OBJECT_0 || stop.stop_requested()) {
                return;
            }

            // 消費済み領域だけを再利用し、音声の補充をゲームのフレーム更新から独立
            for (auto& slot : slots_) {
                if (stop.stop_requested()) {
                    return;
                }
                if (!slot.inFlight.load() && !FillBuffer(voice_.get(), slot)) {
                    failed_.store(true);
                    return;
                }
            }
        }
    }

    /// @brief バッファ終了を変換スレッドへ通知
    /// @param context 消費した出力バッファの識別情報
    void STDMETHODCALLTYPE OnBufferEnd(void* context) override {
        auto* slot = static_cast<BufferSlot*>(context);
        if (slot) {
            slot->inFlight.store(false);
            SetEvent(event_.get());
        }
    }

    /// @brief Voice異常による変換処理の停止を通知
    /// @param context 異常が発生したバッファの識別情報
    /// @param error XAudio2のエラーコード
    void STDMETHODCALLTYPE OnVoiceError(void* context, HRESULT error) override {
        (void)context;
        (void)error;
        failed_.store(true);
        SetEvent(event_.get());
    }

    /// @brief 処理開始通知の受信、追加処理なし
    /// @param bytesRequired 必要な入力バイト数
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 bytesRequired) override { (void)bytesRequired; }
    /// @brief 処理終了通知の受信、追加処理なし
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    /// @brief ストリーム終了通知の受信、追加処理なし
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    /// @brief バッファ開始通知の受信、追加処理なし
    /// @param context 開始したバッファの識別情報
    void STDMETHODCALLTYPE OnBufferStart(void* context) override { (void)context; }
    /// @brief ループ終了通知の受信、追加処理なし
    /// @param context ループ対象バッファの識別情報
    void STDMETHODCALLTYPE OnLoopEnd(void* context) override { (void)context; }

    std::shared_ptr<const std::vector<BYTE>> pcm_;
    WAVEFORMATEX format_{};
    bool loop_ = false;
    bool isPaused_ = false;
    float volume_ = 1.0f;
    IXAudio2* engine_ = nullptr;
    UniqueEvent event_;
    std::unique_ptr<AudioPitchStream> stream_;
    std::array<BufferSlot, kStreamBuffers> slots_;
    std::atomic<float> pitch_{ 0.0f };
    std::atomic<bool> inputEnded_{ false };
    std::atomic<bool> failed_{ false };
    size_t generatedFrames_ = 0;
    size_t remainingFrames_ = 0;
    UniqueSourceVoice voice_;
    std::jthread worker_;
};

AudioX::AudioX() = default;

AudioX::~AudioX() {
    Reset();
}

void AudioX::Initialize(const std::string& filePath) {
    Reset();
    filePath_ = filePath;
    HRESULT hr = XAudio2Create(&xAudio2_);
    if (FAILED(hr)) {
        return;
    }
    IXAudio2MasteringVoice* mv = nullptr;
    hr = xAudio2_->CreateMasteringVoice(&mv);
    if (FAILED(hr)) {
        xAudio2_.Reset();
        return;
    }
    mastering_.reset(mv);

    // 停止処理が完了するまで各再生インスタンスとの共有PCMの寿命を保証
    SoundData data = SoundLoadAudio(filePath);
    wfex_ = data.wfex;
    pcmShared_ = std::make_shared<std::vector<BYTE>>(std::move(data.pcm));
}

void AudioX::PlayAudio(float volume, bool loop) {
    if (!xAudio2_ || !mastering_ || !pcmShared_ || pcmShared_->empty() || wfex_.nBlockAlign == 0) {
        return;
    }

    // 再生ごとに独立したVoiceと変換状態を保持して同一音源の同時再生を許容
    volume_ = volume;
    auto instance = std::make_unique<ClipInstance>(pcmShared_, wfex_, loop);
    if (instance->Play(xAudio2_.Get(), volume_ * playbackGain_, pitchSemitones_, isPaused_)) {
        actives_.push_back(std::move(instance));
    } else {
        Logger::Output("Audio : 音声の再生開始に失敗しました: " + filePath_, Logger::Level::Error);
    }
}

void AudioX::SetPaused(bool isPaused) {
    if (isPaused_ == isPaused) {
        return;
    }

    // 後続の再生と音程変換によるVoice再生成にも同じ一時停止状態を適用
    isPaused_ = isPaused;
    for (auto& instance : actives_) {
        instance->SetPaused(isPaused_);
    }
}

void AudioX::Update() {

    // バッファ補充は専用スレッドで継続し、毎フレーム処理は終了インスタンスの回収に限定
    std::erase_if(actives_, [](const auto& instance) { return instance->Finished(); });
}

void AudioX::Reset() {

    // 変換スレッドとVoiceを先に停止してから共有PCMとXAudio2を解放
    actives_.clear();
    pcmShared_.reset();
    mastering_.reset();
    xAudio2_.Reset();
    pitchSemitones_ = 0.0f;
    volume_ = 1.0f;
    playbackGain_ = 1.0f;
    isPaused_ = false;
}

void AudioX::SetVolume(float volume) {
    volume_ = volume;

    // MasterやCategoryの更新でも一時的なフェード係数を維持
    for (auto& instance : actives_) {
        instance->SetVolume(volume_ * playbackGain_);
    }
}

void AudioX::SetPlaybackGain(float gain) {
    if (!std::isfinite(gain)) {
        return;
    }

    // 元の設定音量とフェード係数を分離し、フェード終了時の設定復元を不要化
    playbackGain_ = std::clamp(gain, 0.0f, 1.0f);
    SetVolume(volume_);
}

bool AudioX::SetPitch(float semitones) {
    if (!std::isfinite(semitones) || !AudioPitchStream::SupportsFormat(wfex_)) {
        return false;
    }
    semitones = std::clamp(semitones, AudioPitchStream::kMinSemitones, AudioPitchStream::kMaxSemitones);
    if (semitones == pitchSemitones_) {
        return true;
    }

    // 設定変更時だけ全インスタンスへ通知し、後続の再生にも同じ音程を適用
    for (auto& instance : actives_) {
        if (!instance->SetPitch(semitones)) {
            for (auto& previous : actives_) {
                previous->SetPitch(pitchSemitones_);
            }
            return false;
        }
    }
    pitchSemitones_ = semitones;
    return true;
}

void AudioX::StopAll() {

    // 完全停止後の新規再生へ一時停止状態を持ち越さないため初期化
    actives_.clear();
    isPaused_ = false;
}

void AudioX::StopLatest() {
    if (!actives_.empty()) {
        actives_.pop_back();
    }
}

void AudioX::StopAllLoops() {

    // 単発SEを維持したままループ音声の変換スレッドとVoiceだけを停止
    std::erase_if(actives_, [](const auto& instance) { return instance->IsLooping(); });
}
