#pragma once
#include "ParticleTypes.h"

namespace MadoEngine::Particle {

	/// @brief Particle Emitterの発生形状をDebugLineへ登録するDrawer
	class ParticleEmitterDebugDrawer3d final {
	public:
		/// @brief Emitterの発生形状をDebugLineへ登録する
		/// @param config 表示するEmitter設定
		/// @param transform EmitterのTransform
		/// @param color 発生形状の表示色
		static void Submit(
			const EmitterConfig& config,
			const Transform3D& transform,
			const Vector4& color
		);

	private:
		ParticleEmitterDebugDrawer3d() = delete;
	};

} // namespace MadoEngine::Particle
