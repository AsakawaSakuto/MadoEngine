#pragma once
// Audio.h
// .XAudio2を安全に扱う薄いラッパ。SourceVoiceはunique_ptr＋カスタムデリータで管理。
// 1ファイル読み込み（WAV/MP3）→任意回数Play（同時再生可）。ループにも対応。

#include <memory>
#include <vector>
#include <wrl/client.h>
#include <xaudio2.h>
#include "AudioData.h"

#pragma comment(lib, "xaudio2.lib")
#include <cassert>

// DestroyVoice専用デリータ
struct SourceVoiceDeleter {
    void operator()(IXAudio2SourceVoice* v) const noexcept {
        if (!v) return;
        v->Stop(0);
        v->FlushSourceBuffers();
        v->DestroyVoice();
    }
};

struct MasteringVoiceDeleter {
    void operator()(IXAudio2MasteringVoice* v) const noexcept {
        if (v) v->DestroyVoice();
    }
};

using UniqueSourceVoice = std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>;
using UniqueMasteringVoice = std::unique_ptr<IXAudio2MasteringVoice, MasteringVoiceDeleter>;

// 1回の再生インスタンス（同一PCMを共有）
class ClipInstance {
public:
    // PCM共有バッファを共有所有する
    std::shared_ptr<const std::vector<BYTE>> pcm_;
    WAVEFORMATEX wfex_{};
    UniqueSourceVoice voice_{ nullptr };
    bool loop_ = false;

    // 再生開始（volume範囲は0.0f～1.0f）
    void Play(IXAudio2* xa, float volume = 1.0f) {
        IXAudio2SourceVoice* raw = nullptr;
        HRESULT hr = xa->CreateSourceVoice(&raw, &wfex_);
        assert(SUCCEEDED(hr));
        voice_.reset(raw);
        
        // 音量設定（0.0f～1.0fの範囲にクランプ）
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        voice_->SetVolume(volume);

        XAUDIO2_BUFFER buf{};
        buf.pAudioData = pcm_->data();
        buf.AudioBytes = static_cast<UINT32>(pcm_->size() - (pcm_->size() % wfex_.nBlockAlign));
        buf.Flags = XAUDIO2_END_OF_STREAM;
        if (loop_) {
            DWORD samples = buf.AudioBytes / wfex_.nBlockAlign;
            buf.LoopBegin = 0;
            buf.LoopLength = samples;
            buf.LoopCount = XAUDIO2_LOOP_INFINITE;
        }

        hr = voice_->SubmitSourceBuffer(&buf);
        assert(SUCCEEDED(hr));
        hr = voice_->Start();
        assert(SUCCEEDED(hr));
    }

    // 終了判定
    bool Finished() const {
        if (!voice_) return true;
        XAUDIO2_VOICE_STATE st{};
        voice_->GetState(&st);
        return st.BuffersQueued == 0;
    }

    // 明示停止
    void Stop() {
        if (!voice_) return;
        voice_->Stop(0);
        voice_->FlushSourceBuffers();
    }
};

class AudioX {
public:
    // filePath(.wav/.mp3)を読み込み（PCM16へ）。以後PlayAudioで再生。
    void Initialize(const std::string& filePath);

    // 再生。loop=trueで無限ループ。複数回呼ぶと同時再生可能。volume範囲は0.0f～1.0f
    void PlayAudio(float volume = 1.0f, bool loop = false);

    // 毎フレーム呼ぶ。終了済みインスタンスの破棄を行う。
    void Update();

    // すべて停止して後始末
    void Reset();

    // 全インスタンスに音量を適用（0.0f～1.0f）
    void SetVolume(float volume);

    // 再生中の全インスタンスを即時停止して破棄（PCMとXAudio2は残す）
    void StopAll();

    // 直近に再生開始した1インスタンスだけ止める（任意）
    void StopLatest();

    // ループ再生インスタンスだけ止める（任意）
    void StopAllLoops();

    ~AudioX() { Reset(); }

private:
	std::string filePath_{};

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    UniqueMasteringVoice mastering_{ nullptr };

    // 読み込んだPCMの共有所有
    std::shared_ptr<std::vector<BYTE>> pcmShared_{};
    WAVEFORMATEX wfex_{};

    // 再生中インスタンス
    std::vector<std::unique_ptr<ClipInstance>> actives_;
};
