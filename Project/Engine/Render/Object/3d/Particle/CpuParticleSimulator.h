#pragma once
#include "ParticleTypes.h"
#include "Utility/Random.h"
#include <cstdint>
#include <span>
#include <vector>

namespace MadoEngine::Particle {

	/// @brief CPU上でParticleの生成と更新を行うSimulator
	class CpuParticleSimulator final {
	public:
		/// @brief 固定長ParticlePoolを初期化する
		/// @param maxParticles 最大Particle数
		void Initialize(uint32_t maxParticles);

		/// @brief 全Particleを消去する
		void Reset();

		/// @brief Emitter設定に従ってParticleを生成する
		/// @param config Emitter設定
		/// @param emitterTransform EmitterのTransform
		/// @param count 生成数
		/// @param random 使用する乱数生成器
		void Emit(
			const EmitterConfig& config,
			const Transform3D& emitterTransform,
			uint32_t count,
			Random& random
		);

		/// @brief 生存中Particleを更新する
		/// @param deltaTime 前フレームからの経過時間
		/// @param config Emitter設定
		void Update(float deltaTime, const EmitterConfig& config);

		/// @brief 生存中Particleを取得する
		/// @return 生存中Particleの連続領域
		std::span<const ParticleState> GetParticles() const;

		/// @brief 生存中Particle数を取得する
		/// @return 生存中Particle数
		uint32_t GetAliveCount() const { return aliveCount_; }

	private:
		std::vector<ParticleState> particles_;
		uint32_t aliveCount_ = 0;
	};

} // namespace MadoEngine::Particle
