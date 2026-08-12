#include "ParticleEmitterRuntimeFactory.h"
#include "CpuParticleEmitterRuntime.h"
#include "GpuParticleEmitterRuntime.h"
#include <new>

namespace MadoEngine::Particle {

	void ParticleEmitterRuntimeFactory::Initialize(
		ID3D12Device* device,
		MadoEngine::Render::ComputePSORegistry* computePsoRegistry,
		bool isGpuBackendAvailable) {
		device_ = device;
		computePsoRegistry_ = computePsoRegistry;
		isGpuBackendAvailable_ =
			isGpuBackendAvailable &&
			device_ != nullptr &&
			computePsoRegistry_ != nullptr;
	}

	std::unique_ptr<IParticleEmitterRuntime> ParticleEmitterRuntimeFactory::Create(
		const EmitterConfig& config,
		uint32_t randomSeed,
		const std::optional<ParticleBackend>& backendOverride,
		std::string& outFallbackReason) const {
		outFallbackReason.clear();
		ParticleBackend requestedBackend =
			backendOverride.value_or(config.backend);
		if (requestedBackend == ParticleBackend::Count) {
			requestedBackend = ParticleBackend::Auto;
		}
		const bool requestsGpu =
			requestedBackend == ParticleBackend::Auto ||
			requestedBackend == ParticleBackend::Gpu;

		if (requestsGpu && config.renderer.sortMode == SortMode::BackToFront) {

			// GPU側に未実装の描画順を保つためCPU Backendへ切り替え
			outFallbackReason =
				"奥から手前のGPUソートが未実装のためCPU Backendを使用しています。";
		} else if (requestsGpu && !isGpuBackendAvailable_) {
			outFallbackReason =
				"Compute ShaderまたはGPU Particle基盤を利用できないためCPU Backendを使用しています。";
		} else if (requestsGpu) {

			// Autoを含むGPU要求ではGPU Runtimeを優先し、初期化失敗時だけFallback
			try {
				auto gpuRuntime = std::make_unique<GpuParticleEmitterRuntime>(
					device_,
					computePsoRegistry_
				);
				if (gpuRuntime->Initialize(config, randomSeed)) {
					return gpuRuntime;
				}
			}
			catch (const std::bad_alloc&) {
			}
			outFallbackReason =
				"GPU Bufferの作成に失敗したためCPU Backendを使用しています。";
		}

		// 明示的なCPU要求またはGPU利用不可時の共通Fallback
		try {
			auto cpuRuntime = std::make_unique<CpuParticleEmitterRuntime>();
			if (!cpuRuntime->Initialize(config, randomSeed)) {
				return nullptr;
			}
			return cpuRuntime;
		}
		catch (const std::bad_alloc&) {
			return nullptr;
		}
	}

} // MadoEngine::Particle名前空間
