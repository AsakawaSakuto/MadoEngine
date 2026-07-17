#pragma once
#include "ParticleSystem3d.h"

namespace MyParticle3d {

	/// @brief Particle Effectを再生する
	/// @param assetName 再生するAsset名
	/// @param desc 再生設定
	/// @return 再生中Effect Handle
	inline MadoEngine::Particle::EffectHandle Play(
		const std::string& assetName,
		const MadoEngine::Particle::PlayDesc& desc = {}) {
		return MadoEngine::Particle::ParticleSystem3d::GetInstance().Play(assetName, desc);
	}

	/// @brief Particle Effectを停止する
	/// @param handle 停止するEffect Handle
	/// @param mode 停止方法
	inline void Stop(
		MadoEngine::Particle::EffectHandle handle,
		MadoEngine::Particle::StopMode mode = MadoEngine::Particle::StopMode::Finish) {
		MadoEngine::Particle::ParticleSystem3d::GetInstance().Stop(handle, mode);
	}

	/// @brief Particle EffectのTransformを変更する
	/// @param handle 変更するEffect Handle
	/// @param transform 設定するTransform
	/// @return 変更に成功した場合はtrue
	inline bool SetTransform(
		MadoEngine::Particle::EffectHandle handle,
		const Transform3D& transform) {
		return MadoEngine::Particle::ParticleSystem3d::GetInstance().SetTransform(handle, transform);
	}

} // namespace MyParticle3d
