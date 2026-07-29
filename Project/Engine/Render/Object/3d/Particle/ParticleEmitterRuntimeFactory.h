#pragma once
#include "IParticleEmitterRuntime.h"
#include <memory>
#include <optional>
#include <string>

struct ID3D12Device;

namespace MadoEngine::Render {
	class ComputePSORegistry;
}

namespace MadoEngine::Particle {

	/// @brief Emitter設定に応じてCPU/GPU Runtimeを生成するFactory
	class ParticleEmitterRuntimeFactory final {
	public:
		/// @brief Factoryを初期化する
		/// @param device D3D12 Device
		/// @param computePsoRegistry Compute PSO Registry
		/// @param isGpuBackendAvailable GPU Backendを利用できる場合はtrue
		void Initialize(
			ID3D12Device* device,
			MadoEngine::Render::ComputePSORegistry* computePsoRegistry,
			bool isGpuBackendAvailable
		);

		/// @brief Emitter Runtimeを生成する
		/// @param config Emitter設定
		/// @param randomSeed Emitter専用乱数Seed
		/// @param backendOverride 再生単位のBackend上書き
		/// @param outFallbackReason CPUへフォールバックした理由
		/// @return 初期化済みRuntime
		std::unique_ptr<IParticleEmitterRuntime> Create(
			const EmitterConfig& config,
			uint32_t randomSeed,
			const std::optional<ParticleBackend>& backendOverride,
			std::string& outFallbackReason
		) const;

		/// @brief GPU Backendを利用できるか取得する
		/// @return 利用できる場合はtrue
		bool IsGpuBackendAvailable() const { return isGpuBackendAvailable_; }

	private:
		ID3D12Device* device_ = nullptr;
		MadoEngine::Render::ComputePSORegistry* computePsoRegistry_ = nullptr;
		bool isGpuBackendAvailable_ = false;
	};

} // MadoEngine::Particle名前空間
