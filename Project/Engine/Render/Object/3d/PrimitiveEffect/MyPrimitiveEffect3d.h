#pragma once
#include "PrimitiveEffectSystem3d.h"

namespace MyPrimitiveEffect3d {

	/// @brief Cylinderエフェクトを再生する
	/// @param assetName 再生するAsset名
	/// @param desc 再生設定
	/// @return 再生中Instanceを指すHandle
	inline MadoEngine::Effect::PrimitiveEffectHandle Play(
		const std::string& assetName,
		const MadoEngine::Effect::PrimitiveEffectPlayDesc& desc = {}) {
		return MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Play(assetName, desc);
	}

	/// @brief Cylinderエフェクトを停止する
	/// @param handle 停止するInstance Handle
	/// @param mode 停止方法
	inline void Stop(
		MadoEngine::Effect::PrimitiveEffectHandle handle,
		MadoEngine::Effect::PrimitiveEffectStopMode mode = MadoEngine::Effect::PrimitiveEffectStopMode::Finish) {
		MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Stop(handle, mode);
	}

	/// @brief CylinderエフェクトのTransformを変更する
	/// @param handle 変更するInstance Handle
	/// @param transform 設定するTransform
	/// @return 変更に成功した場合はtrue
	inline bool SetTransform(
		MadoEngine::Effect::PrimitiveEffectHandle handle,
		const Transform3D& transform) {
		return MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().SetTransform(handle, transform);
	}

} // namespace MyPrimitiveEffect3d
