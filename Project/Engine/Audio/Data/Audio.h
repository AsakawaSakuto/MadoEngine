#pragma once

#include "AudioData.h"
#include <memory>
#include <vector>
#include <wrl/client.h>

#pragma comment(lib, "xaudio2.lib")

/// @brief SourceVoiceを停止して解放するデリーター
struct SourceVoiceDeleter {
    /// @brief 再生を停止してSourceVoiceを解放
    /// @param voice 解放対象のSourceVoice
    void operator()(IXAudio2SourceVoice* voice) const noexcept {
        if (voice) {
            voice->Stop(0);
            voice->FlushSourceBuffers();
            voice->DestroyVoice();
        }
    }
};

/// @brief MasteringVoiceを解放するデリーター
struct MasteringVoiceDeleter {
    /// @brief MasteringVoiceを解放
    /// @param voice 解放対象のMasteringVoice
    void operator()(IXAudio2MasteringVoice* voice) const noexcept {
        if (voice) {
            voice->DestroyVoice();
        }
    }
};

using UniqueSourceVoice = std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>;
using UniqueMasteringVoice = std::unique_ptr<IXAudio2MasteringVoice, MasteringVoiceDeleter>;

class ClipInstance;

/// @brief WAVとMP3の共有PCMおよび独立した再生インスタンスの管理
class AudioX {
public:
    /// @brief 再生環境を生成
    AudioX();
    /// @brief 再生と変換処理を停止してリソースを解放
    ~AudioX();

    /// @brief WAVまたはMP3を読み込んで再生環境を初期化
    /// @param filePath 音声ファイルのパス
    void Initialize(const std::string& filePath);

    /// @brief 現在の音程設定で独立した再生インスタンスを開始
    /// @param volume 音量、0から1の範囲
    /// @param loop ループ再生する場合はtrue
    void PlayAudio(float volume = 1.0f, bool loop = false);

    /// @brief 再生終了したインスタンスを解放
    void Update();

    /// @brief 再生と変換処理を停止してリソースを解放
    void Reset();

    /// @brief 再生中の全インスタンスへ音量を適用
    /// @param volume 音量、0から1の範囲
    void SetVolume(float volume);

    /// @brief 再生速度を維持した音程変更を現在および後続の再生へ適用
    /// @param semitones 元の音程からの半音数、-12から12の範囲
    /// @return 適用に成功した場合はtrue
    bool SetPitch(float semitones);

    /// @brief 現在の音程設定を取得
    /// @return 元の音程からの半音数
    float GetPitch() const { return pitchSemitones_; }

    /// @brief PCMを保持したまま全インスタンスの再生を停止
    void StopAll();

    /// @brief 最後に開始したインスタンスの再生を停止
    void StopLatest();

    /// @brief ループ再生中のインスタンスを停止
    void StopAllLoops();

    /// @brief 再生中のインスタンスの有無を取得
    /// @return 再生中のインスタンスが存在する場合はtrue
    bool IsPlaying() const { return !actives_.empty(); }

private:
    std::string filePath_;
    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    UniqueMasteringVoice mastering_;
    std::shared_ptr<const std::vector<BYTE>> pcmShared_;
    WAVEFORMATEX wfex_{};
    float pitchSemitones_ = 0.0f;
    std::vector<std::unique_ptr<ClipInstance>> actives_;
};
