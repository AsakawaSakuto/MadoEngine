#pragma once

#include "AudioData.h"
#include <memory>

/// @brief 再生時間を維持したPCMの音程変換ストリーム
class AudioPitchStream {
public:
    static constexpr float kMinSemitones = -12.0f;
    static constexpr float kMaxSemitones = 12.0f;

    /// @brief 音程変換ストリームを生成
    AudioPitchStream();
    /// @brief 音程変換の作業領域を解放
    ~AudioPitchStream();
    AudioPitchStream(const AudioPitchStream&) = delete;
    AudioPitchStream& operator=(const AudioPitchStream&) = delete;

    /// @brief PCM形式が音程変換に対応しているか確認
    /// @param format PCMの形式
    /// @return 対応している場合はtrue
    static bool SupportsFormat(const WAVEFORMATEX& format);

    /// @brief PCMを共有して指定位置から音程変換を初期化
    /// @param pcm 元のPCMバッファ
    /// @param format PCMの形式
    /// @param semitones 元の音程からの半音数、-12から12の範囲
    /// @param loop ループ再生する場合はtrue
    /// @param startFrame 再生開始位置のサンプルフレーム番号
    /// @return 初期化に成功した場合はtrue
    bool Initialize(std::shared_ptr<const std::vector<BYTE>> pcm, const WAVEFORMATEX& format,
        float semitones, bool loop, size_t startFrame = 0);

    /// @brief 後続の音程変換に適用する半音数を設定
    /// @param semitones 元の音程からの半音数、-12から12の範囲
    void SetPitch(float semitones);

    /// @brief 元の時間軸に対応するインターリーブ形式のFloat32音声を取得
    /// @param output maxFramesとチャンネル数の積以上の要素を持つ出力先
    /// @param maxFrames 取得する最大サンプルフレーム数
    /// @return 取得したフレーム数、終端の場合は0
    size_t Read(float* output, size_t maxFrames);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
