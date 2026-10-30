#include "AudioLoudnessAnalyzer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numbers>

namespace {
    constexpr double kLoudnessOffset = -0.691;
    constexpr double kAbsoluteGateLufs = -70.0;
    constexpr double kRelativeGateEnergyRatio = 0.1;
    constexpr double kBlockSeconds = 0.4;
    constexpr double kHopSeconds = 0.1;

    /// @brief 二次IIRフィルターの正規化済み係数
    struct BiquadCoefficients {
        double b0 = 1.0;
        double b1 = 0.0;
        double b2 = 0.0;
        double a1 = 0.0;
        double a2 = 0.0;
    };

    /// @brief 単一チャンネルの独立した履歴を保持する二次IIRフィルター
    class BiquadFilter {
    public:
        /// @brief フィルター係数を設定
        /// @param coefficients 正規化済みフィルター係数
        explicit BiquadFilter(const BiquadCoefficients& coefficients) : coefficients_(coefficients) {}

        /// @brief 一サンプルをフィルターへ適用
        /// @param sample 入力サンプル
        /// @return 周波数補正後のサンプル
        double Process(double sample) {

            // 転置直接形IIの二つの遅延だけで二次IIRの履歴を更新
            const double output = coefficients_.b0 * sample + delay1_;
            delay1_ = coefficients_.b1 * sample - coefficients_.a1 * output + delay2_;
            delay2_ = coefficients_.b2 * sample - coefficients_.a2 * output;
            return output;
        }

    private:
        BiquadCoefficients coefficients_;
        double delay1_ = 0.0;
        double delay2_ = 0.0;
    };

    /// @brief 音声形式の解析対応状況を判定
    /// @param format 確認する音声形式
    /// @return 安全にラウドネスを解析できる場合はtrue
    bool IsSupportedFormat(const WAVEFORMATEX& format) {

        // チャンネル配置が不明な多チャンネル音源を誤った重みで評価しない制約
        if (format.nChannels < 1 || format.nChannels > 2 ||
            format.nSamplesPerSec < 8000 || format.nSamplesPerSec > 384000) {
            return false;
        }

        const std::size_t bytesPerSample = format.wBitsPerSample / 8;
        if (bytesPerSample == 0 || format.nBlockAlign < format.nChannels * bytesPerSample) {
            return false;
        }

        // 再生用Decoderと同じ整数PCMおよびFloat32のサンプル表現を受け付け
        if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            return format.wBitsPerSample == 32;
        }
        if (format.wFormatTag == WAVE_FORMAT_PCM) {
            return format.wBitsPerSample == 8 || format.wBitsPerSample == 16 ||
                format.wBitsPerSample == 24 || format.wBitsPerSample == 32;
        }
        return false;
    }

    /// @brief 音声サンプルをフルスケール基準の振幅へ変換
    /// @param sample サンプル先頭のアドレス
    /// @param format 音声形式
    /// @return 正規化した振幅、非有限のFloat値の場合は0
    double DecodeSample(const BYTE* sample, const WAVEFORMATEX& format) {
        if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            float value = 0.0f;
            std::memcpy(&value, sample, sizeof(value));

            // 非有限値をフィルター履歴へ伝播させず、以後の全測定値の破損を防止
            if (!std::isfinite(value)) {
                return 0.0;
            }
            return static_cast<double>(value);
        }

        // バイト境界や符号表現に依存せず、量子化幅ごとのゼロ点と最大振幅を復元
        switch (format.wBitsPerSample) {
        case 8:
            return (static_cast<double>(*sample) - 128.0) / 128.0;
        case 16: {
            std::int16_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            return static_cast<double>(value) / 32768.0;
        }
        case 24: {
            std::int32_t value = sample[0] | (sample[1] << 8) | (sample[2] << 16);
            if ((value & 0x800000) != 0) {
                value -= 0x1000000;
            }
            return static_cast<double>(value) / 8388608.0;
        }
        case 32: {
            std::int32_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            return static_cast<double>(value) / 2147483648.0;
        }
        default:
            return 0.0;
        }
    }

    /// @brief K-weightingの高域シェルフ係数を音源のサンプルレートへ変換
    /// @param sampleRate 音源のサンプルレート
    /// @return 高域シェルフの正規化済み係数
    BiquadCoefficients CreateShelfCoefficients(double sampleRate) {
        constexpr double kFrequency = 1681.974450955533;
        constexpr double kQuality = 0.7071752369554196;
        constexpr double kGainDb = 3.999843853973347;
        const double warpedFrequency = std::tan(std::numbers::pi * kFrequency / sampleRate);
        const double squaredFrequency = warpedFrequency * warpedFrequency;
        const double damping = warpedFrequency / kQuality;
        const double highGain = std::pow(10.0, kGainDb / 20.0);
        const double bandGain = std::pow(highGain, 0.4996667741545416);
        const double denominator = 1.0 + damping + squaredFrequency;

        // 48kHzの基準応答を他のサンプルレートでも維持するため周波数をプリワープ
        return {
            (highGain + bandGain * damping + squaredFrequency) / denominator,
            2.0 * (squaredFrequency - highGain) / denominator,
            (highGain - bandGain * damping + squaredFrequency) / denominator,
            2.0 * (squaredFrequency - 1.0) / denominator,
            (1.0 - damping + squaredFrequency) / denominator,
        };
    }

    /// @brief K-weightingの低域除去係数を音源のサンプルレートへ変換
    /// @param sampleRate 音源のサンプルレート
    /// @return 低域除去フィルターの正規化済み係数
    BiquadCoefficients CreateHighPassCoefficients(double sampleRate) {
        constexpr double kFrequency = 38.13547087602444;
        constexpr double kQuality = 0.5003270373238773;
        const double warpedFrequency = std::tan(std::numbers::pi * kFrequency / sampleRate);
        const double squaredFrequency = warpedFrequency * warpedFrequency;
        const double damping = warpedFrequency / kQuality;
        const double denominator = 1.0 + damping + squaredFrequency;

        // BS.1770の基準応答に合わせ、分子係数は1、-2、1を維持
        return { 1.0, -2.0, 1.0,
            2.0 * (squaredFrequency - 1.0) / denominator,
            (1.0 - damping + squaredFrequency) / denominator };
    }

    /// @brief 二段階のゲートを通過したブロックからIntegrated LUFSを算出
    /// @param blockEnergies K-weighting適用後の400msブロック平均二乗値
    /// @return Integrated LUFS、有効ブロックがない場合は負の無限大
    double CalculateIntegratedLufs(const std::vector<double>& blockEnergies) {
        const double absoluteThreshold = std::pow(10.0, (kAbsoluteGateLufs - kLoudnessOffset) / 10.0);
        double absoluteEnergySum = 0.0;
        std::size_t absoluteCount = 0;

        // 無音と非常に小さい信号を除外して相対ゲートの基準音量を決定
        for (double energy : blockEnergies) {
            if (energy > absoluteThreshold) {
                absoluteEnergySum += energy;
                ++absoluteCount;
            }
        }
        if (absoluteCount == 0) {
            return -std::numeric_limits<double>::infinity();
        }

        const double relativeThreshold = absoluteEnergySum / static_cast<double>(absoluteCount) * kRelativeGateEnergyRatio;
        const double threshold = (std::max)(absoluteThreshold, relativeThreshold);
        double gatedEnergySum = 0.0;
        std::size_t gatedCount = 0;

        // 基準より10dB以上小さい区間を除外し、長い無音部分による見かけの音量低下を抑制
        for (double energy : blockEnergies) {
            if (energy > threshold) {
                gatedEnergySum += energy;
                ++gatedCount;
            }
        }
        if (gatedCount == 0) {
            return -std::numeric_limits<double>::infinity();
        }
        return kLoudnessOffset + 10.0 * std::log10(gatedEnergySum / static_cast<double>(gatedCount));
    }
}

namespace MadoEngine {

    AudioLoudnessMetrics AudioLoudnessAnalyzer::Analyze(const std::vector<BYTE>& pcm, const WAVEFORMATEX& format) {
        AudioLoudnessMetrics result;
        if (!IsSupportedFormat(format) || pcm.size() < format.nBlockAlign) {
            return result;
        }

        const std::size_t frameCount = pcm.size() / format.nBlockAlign;
        const std::size_t bytesPerSample = format.wBitsPerSample / 8;
        const std::size_t blockFrames = static_cast<std::size_t>(std::llround(format.nSamplesPerSec * kBlockSeconds));
        const std::size_t hopFrames = static_cast<std::size_t>(std::llround(format.nSamplesPerSec * kHopSeconds));
        const BiquadCoefficients shelf = CreateShelfCoefficients(format.nSamplesPerSec);
        const BiquadCoefficients highPass = CreateHighPassCoefficients(format.nSamplesPerSec);
        std::array<BiquadFilter, 2> shelfFilters = { BiquadFilter(shelf), BiquadFilter(shelf) };
        std::array<BiquadFilter, 2> highPassFilters = { BiquadFilter(highPass), BiquadFilter(highPass) };
        std::vector<double> energyWindow(blockFrames, 0.0);
        std::vector<double> blockEnergies;
        blockEnergies.reserve(frameCount / hopFrames + 1);
        std::size_t windowPosition = 0;
        std::size_t framesUntilBlock = blockFrames;
        double windowEnergy = 0.0;
        double peakAmplitude = 0.0;
        result.isValid = true;
        result.isShortClip = frameCount < blockFrames;

        // 短いSEも比較できるよう400msまで無音を補完し、通常のIntegrated LUFSとは参考値として区別
        const std::size_t analysisFrames = (std::max)(frameCount, blockFrames);
        for (std::size_t frame = 0; frame < analysisFrames; ++frame) {
            double frameEnergy = 0.0;
            for (std::size_t channel = 0; channel < format.nChannels; ++channel) {
                double sample = 0.0;
                if (frame < frameCount) {
                    sample = DecodeSample(pcm.data() + frame * format.nBlockAlign + channel * bytesPerSample, format);
                    peakAmplitude = (std::max)(peakAmplitude, std::abs(sample));
                }

                // モノラルとステレオは各チャンネルの重みを1としてエネルギーを合算
                const double weightedSample = highPassFilters[channel].Process(shelfFilters[channel].Process(sample));
                frameEnergy += weightedSample * weightedSample;
            }

            // 400ms分だけを保持する循環窓で、音源の長さに比例するサンプル複製を回避
            windowEnergy += frameEnergy - energyWindow[windowPosition];
            energyWindow[windowPosition] = frameEnergy;
            ++windowPosition;
            if (windowPosition == blockFrames) {
                windowPosition = 0;
            }
            --framesUntilBlock;
            if (framesUntilBlock == 0) {

                // 75%の重複で完全なブロックだけを採用し、末尾の不完全区間はPeak測定だけへ反映
                blockEnergies.push_back((std::max)(0.0, windowEnergy / static_cast<double>(blockFrames)));
                framesUntilBlock = hopFrames;
            }
        }

        result.integratedLufs = CalculateIntegratedLufs(blockEnergies);
        if (peakAmplitude > 0.0) {
            result.samplePeakDbfs = 20.0 * std::log10(peakAmplitude);
        }
        return result;
    }

    AudioLoudnessMetrics AudioLoudnessAnalyzer::EstimateAfterGain(const AudioLoudnessMetrics& source, float gain) {
        AudioLoudnessMetrics result = source;

        // 無音や不正な倍率では対数を計算せず、表示へNaNを伝播させない制約
        if (!std::isfinite(gain) || gain <= 0.0f) {
            result.integratedLufs = -std::numeric_limits<double>::infinity();
            result.samplePeakDbfs = -std::numeric_limits<double>::infinity();
            return result;
        }

        // 毎フレームの再解析を避け、元音源の測定値へ個別音量のdB差だけを反映
        const double gainDb = 20.0 * std::log10(static_cast<double>(gain));
        result.integratedLufs += gainDb;
        result.samplePeakDbfs += gainDb;
        return result;
    }
}
