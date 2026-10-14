#include "AudioPitchStream.h"
#include <algorithm>
#include <cmath>
#include <cstring>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#pragma warning(push, 0)
#include "SignalsmithStretch/signalsmith-stretch.h"
#pragma warning(pop)
#pragma pop_macro("max")
#pragma pop_macro("min")

namespace {
    constexpr size_t kProcessFrames = 1024;

    /// @brief インターリーブ音声の単一チャンネル参照
    struct ChannelView {
        float* samples;
        size_t channels;

        /// @brief 対象チャンネルのサンプルを参照
        /// @param frame サンプルフレーム番号
        /// @return サンプルへの参照
        float& operator[](size_t frame) const {
            return samples[frame * channels];
        }
    };

    /// @brief ライブラリへチャンネル単位の参照を提供するアダプター
    struct InterleavedView {
        float* samples;
        size_t channels;

        /// @brief チャンネル参照を取得
        /// @param channel チャンネル番号
        /// @return 対象チャンネルの参照
        ChannelView operator[](size_t channel) const {
            return { samples + channel, channels };
        }
    };

    /// @brief PCMサンプルを正規化したFloat32へ変換
    /// @param sample サンプル先頭のアドレス
    /// @param format PCMの形式
    /// @return 正規化したサンプル値
    float DecodeSample(const BYTE* sample, const WAVEFORMATEX& format) {

        // 非有限のFloat音声によるFFT全体への異常値伝播を防止
        if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            float value = 0.0f;
            std::memcpy(&value, sample, sizeof(value));
            if (!std::isfinite(value)) {
                return 0.0f;
            }
            return value;
        }

        // PCMの量子化幅ごとに符号とゼロ点を復元して共通の振幅範囲へ変換
        switch (format.wBitsPerSample) {
        case 8:
            return (static_cast<float>(*sample) - 128.0f) / 128.0f;
        case 16: {
            int16_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            return static_cast<float>(value) / 32768.0f;
        }
        case 24: {
            int32_t value = sample[0] | (sample[1] << 8) | (sample[2] << 16);
            if ((value & 0x800000) != 0) {
                value -= 0x1000000;
            }
            return static_cast<float>(value) / 8388608.0f;
        }
        case 32: {
            int32_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            return static_cast<float>(static_cast<double>(value) / 2147483648.0);
        }
        default:
            return 0.0f;
        }
    }
}

class AudioPitchStream::Impl {
public:
    signalsmith::stretch::SignalsmithStretch<float> stretch;
    std::shared_ptr<const std::vector<BYTE>> pcm;
    WAVEFORMATEX format{};
    std::vector<float> input;
    std::vector<float> discarded;
    size_t inputFrame = 0;
    size_t totalFrames = 0;
    size_t remainingFrames = 0;
    bool loop = false;

    /// @brief 元PCMの後続フレームをFloat32の作業領域へ読み込み
    /// @param frames 読み込むフレーム数
    void ReadInput(size_t frames) {
        const size_t channels = format.nChannels;
        const size_t bytesPerSample = format.wBitsPerSample / 8;

        // ループ境界は連続入力として扱い、単発音声の終端以降は遅延回収用の無音を供給
        for (size_t frame = 0; frame < frames; ++frame) {
            if (inputFrame == totalFrames && loop) {
                inputFrame = 0;
            }
            for (size_t channel = 0; channel < channels; ++channel) {
                float value = 0.0f;
                if (inputFrame < totalFrames) {
                    const BYTE* sample = pcm->data() + inputFrame * format.nBlockAlign + channel * bytesPerSample;
                    value = DecodeSample(sample, format);
                }
                input[frame * channels + channel] = value;
            }
            if (inputFrame < totalFrames) {
                ++inputFrame;
            }
        }
    }

    /// @brief 時間倍率を1に固定して後続フレームを音程変換
    /// @param output Float32の出力先
    /// @param frames 入出力共通のフレーム数
    void Process(float* output, size_t frames) {
        ReadInput(frames);

        // 入出力フレーム数の一致により音程だけを変更して再生速度を維持
        stretch.process(InterleavedView{ input.data(), format.nChannels }, static_cast<int>(frames),
            InterleavedView{ output, format.nChannels }, static_cast<int>(frames));
    }
};

AudioPitchStream::AudioPitchStream() = default;
AudioPitchStream::~AudioPitchStream() = default;

bool AudioPitchStream::SupportsFormat(const WAVEFORMATEX& format) {
    if (format.nChannels == 0 || format.nChannels > XAUDIO2_MAX_AUDIO_CHANNELS ||
        format.nSamplesPerSec < XAUDIO2_MIN_SAMPLE_RATE || format.nSamplesPerSec > XAUDIO2_MAX_SAMPLE_RATE ||
        format.nBlockAlign != format.nChannels * (format.wBitsPerSample / 8)) {
        return false;
    }

    // WAVの整数PCMとFloat32、およびMP3から復号したPCM16を共通の経路へ集約
    if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        return format.wBitsPerSample == 32;
    }
    if (format.wFormatTag == WAVE_FORMAT_PCM) {
        return format.wBitsPerSample == 8 || format.wBitsPerSample == 16 ||
            format.wBitsPerSample == 24 || format.wBitsPerSample == 32;
    }
    return false;
}

bool AudioPitchStream::Initialize(std::shared_ptr<const std::vector<BYTE>> pcm, const WAVEFORMATEX& format,
    float semitones, bool loop, size_t startFrame) {
    if (!pcm || !SupportsFormat(format) || !std::isfinite(semitones)) {
        return false;
    }
    const size_t totalFrames = pcm->size() / format.nBlockAlign;
    if (totalFrames == 0 || startFrame >= totalFrames) {
        return false;
    }

    // 初期化失敗時に以前のストリームを保持できるよう新しい状態を別途構築
    auto next = std::make_unique<Impl>();
    next->pcm = std::move(pcm);
    next->format = format;
    next->inputFrame = startFrame;
    next->totalFrames = totalFrames;
    next->remainingFrames = totalFrames - startFrame;
    next->loop = loop;
    next->input.resize(kProcessFrames * format.nChannels);
    next->discarded.resize(kProcessFrames * format.nChannels);
    next->stretch.presetDefault(format.nChannels, static_cast<float>(format.nSamplesPerSec));
    next->stretch.setTransposeSemitones(std::clamp(semitones, kMinSemitones, kMaxSemitones));

    // 先読みで初期遅延を除去し、終端では同じ量の無音入力から末尾を回収
    size_t preRoll = static_cast<size_t>(next->stretch.inputLatency() + next->stretch.outputLatency());
    while (preRoll > 0) {
        const size_t frames = (std::min)(preRoll, kProcessFrames);
        next->Process(next->discarded.data(), frames);
        preRoll -= frames;
    }
    impl_ = std::move(next);
    return true;
}

void AudioPitchStream::SetPitch(float semitones) {
    if (impl_ && std::isfinite(semitones)) {
        impl_->stretch.setTransposeSemitones(std::clamp(semitones, kMinSemitones, kMaxSemitones));
    }
}

size_t AudioPitchStream::Read(float* output, size_t maxFrames) {
    if (!impl_ || !output) {
        return 0;
    }
    size_t frames = maxFrames;
    if (!impl_->loop) {
        frames = (std::min)(frames, impl_->remainingFrames);
    }

    // 作業領域を一定に保ち、音源の長さによる一時メモリ増加を回避
    size_t written = 0;
    while (written < frames) {
        const size_t blockFrames = (std::min)(frames - written, kProcessFrames);
        impl_->Process(output + written * impl_->format.nChannels, blockFrames);
        written += blockFrames;
    }
    if (!impl_->loop) {
        impl_->remainingFrames -= written;
    }
    return written;
}
