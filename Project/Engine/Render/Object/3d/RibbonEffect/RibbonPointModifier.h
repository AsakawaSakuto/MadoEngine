#pragma once
#include "RibbonEffectTypes.h"

namespace MadoEngine::Ribbon {

	/// @brief 生成方式に依存しないRibbon点列の変形処理
	class RibbonPointModifier final {
	public:
		/// @brief 点の寿命と元座標を保持した描画用点列へNoiseを付与
		/// @param points 描画用点列
		/// @param noise Noise設定
		/// @param elapsedTime 再生開始からの経過時間
		void ApplyNoise(std::vector<RibbonPoint>& points, const RibbonNoiseModule& noise, float elapsedTime) const;

	private:
		/// @brief 格子Hashを補間した連続Noiseを評価
		/// @param coordinate Noise座標
		/// @param seed Noiseの乱数Seed
		/// @return -1から1のNoise値
		float EvaluateNoise(float coordinate, uint32_t seed) const;
	};

} // namespace MadoEngine::Ribbon
