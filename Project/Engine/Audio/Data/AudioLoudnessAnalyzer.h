#pragma once
#include "AudioData.h"
#include <limits>

namespace MadoEngine {

    /// @brief 音源全体のラウドネスとサンプルピークの解析結果
    struct AudioLoudnessMetrics {
        double integratedLufs = -std::numeric_limits<double>::infinity();
        double samplePeakDbfs = -std::numeric_limits<double>::infinity();
        bool isValid = false;       // 対応する音声形式を解析できた場合はtrue
        bool isShortClip = false;   // 400ms未満の音源へ無音を補完した場合はtrue
    };

    /// @brief 音声の再生処理から独立したLUFSとPeakの解析
    class AudioLoudnessAnalyzer {
    public:
        /// @brief モノラルまたはステレオのPCMからK-weightingと二段階ゲートによるLUFSを算出
        /// @param pcm インターリーブされた音声サンプル
        /// @param format PCMまたはFloat32の音声形式
        /// @return LUFSとサンプルピーク、400ms未満の場合は無音補完による参考LUFS
        static AudioLoudnessMetrics Analyze(const std::vector<BYTE>& pcm, const WAVEFORMATEX& format);

        /// @brief 元音源の解析結果へ個別音量倍率を反映した目安を算出
        /// @param source 元音源の解析結果
        /// @param gain 適用する音量倍率、0以下の場合は無音
        /// @return ゲートの再判定と音程変更を含まない音量調整後の推定値
        static AudioLoudnessMetrics EstimateAfterGain(const AudioLoudnessMetrics& source, float gain);
    };
}
