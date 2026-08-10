#pragma once
#include "RibbonEffectSystem3d.h"

namespace MyRibbonEffect3d {

	/// @brief Ribbon Effectを再生
	/// @param assetName 再生するAsset名
	/// @param desc 再生設定
	/// @return 再生中Instance Handle
	inline MadoEngine::Ribbon::RibbonEffectHandle Play(
		const std::string& assetName,
		const MadoEngine::Ribbon::RibbonEffectPlayDesc& desc = {}) {
		return MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Play(assetName, desc);
	}

	/// @brief Ribbon Effectを停止
	/// @param handle 停止対象Handle
	/// @param mode 停止方式
	inline void Stop(
		MadoEngine::Ribbon::RibbonEffectHandle handle,
		MadoEngine::Ribbon::RibbonStopMode mode = MadoEngine::Ribbon::RibbonStopMode::Finish) {
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Stop(handle, mode);
	}

	/// @brief Ribbon追跡Transformを更新
	/// @param handle 更新対象Handle
	/// @param transform 最新Transform
	/// @return 更新に成功した場合はtrue
	inline bool SetTransform(
		MadoEngine::Ribbon::RibbonEffectHandle handle,
		const Transform3D& transform) {
		return MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetTransform(handle, transform);
	}

	/// @brief Manual Ribbonの制御点を置換
	/// @param handle 更新対象Handle
	/// @param controlPoints 設定順の制御点
	/// @return 更新に成功した場合はtrue
	inline bool SetControlPoints(
		MadoEngine::Ribbon::RibbonEffectHandle handle,
		const std::vector<Vector3>& controlPoints) {
		return MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetControlPoints(handle, controlPoints);
	}

	/// @brief Manual Ribbonの制御点を消去
	/// @param handle 更新対象Handle
	/// @return 消去に成功した場合はtrue
	inline bool ClearControlPoints(MadoEngine::Ribbon::RibbonEffectHandle handle) {
		return MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().ClearControlPoints(handle);
	}

} // namespace MyRibbonEffect3d
