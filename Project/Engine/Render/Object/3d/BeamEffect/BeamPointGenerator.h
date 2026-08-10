#pragma once
#include "BeamEffectTypes.h"
#include <vector>

namespace MadoEngine::Beam {

	/// @brief 始点と終点からBeam用Ribbon Point列を生成する
	class BeamPointGenerator final {
	public:
		/// @brief Beam用Point列を生成
		/// @param startPosition 始点
		/// @param endPosition 終点
		/// @param geometry 形状設定
		/// @param noise Noise設定
		/// @param elapsedTime 再生開始からの総経過時間
		/// @return 始点から終点の順に並んだRibbon Point列
		std::vector<MadoEngine::Ribbon::RibbonPoint> Generate(
			const Vector3& startPosition,
			const Vector3& endPosition,
			const BeamGeometryModule& geometry,
			const BeamNoiseModule& noise,
			float elapsedTime
		) const;

	private:
		/// @brief 1次元の連続したValue Noiseを評価
		/// @param coordinate Noise座標
		/// @param seed Noise Seed
		/// @return -1から1のNoise値
		float EvaluateNoise(float coordinate, uint32_t seed) const;
	};

} // namespace MadoEngine::Beam
