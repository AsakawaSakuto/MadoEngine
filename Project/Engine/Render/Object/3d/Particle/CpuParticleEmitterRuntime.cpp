#include "CpuParticleEmitterRuntime.h"
#include "ParticleRenderer3d.h"
#include <algorithm>

namespace MadoEngine::Particle {

	bool CpuParticleEmitterRuntime::Initialize(const EmitterConfig& config, uint32_t randomSeed) {
		config_ = config;
		config_.emission.maxParticles = std::clamp(
			config_.emission.maxParticles,
			1u,
			kMaximumParticleCountPerEmitter
		);
		random_.SetSeed(randomSeed);
		simulator_.Initialize(config_.emission.maxParticles);
		return true;
	}

	void CpuParticleEmitterRuntime::Update(float deltaTime, const Transform3D& emitterTransform) {
		(void)emitterTransform;
		simulator_.Update(deltaTime, config_);
	}

	void CpuParticleEmitterRuntime::Emit(uint32_t count, const Transform3D& emitterTransform) {
		simulator_.Emit(config_, emitterTransform, count, random_);
	}

	void CpuParticleEmitterRuntime::SetTransform(const Transform3D& emitterTransform) {
		(void)emitterTransform;
	}

	void CpuParticleEmitterRuntime::Stop(StopMode mode) {
		if (mode == StopMode::Immediate) {
			simulator_.Reset();
		}
	}

	void CpuParticleEmitterRuntime::Reset() {
		simulator_.Reset();
	}

	void CpuParticleEmitterRuntime::RecordGpuSimulation(
		ID3D12GraphicsCommandList* commandList,
		uint64_t submissionFenceValue) {
		(void)commandList;
		(void)submissionFenceValue;
	}

	void CpuParticleEmitterRuntime::OnGpuFrameCompleted(uint64_t completedFenceValue) {
		(void)completedFenceValue;
	}

	void CpuParticleEmitterRuntime::SubmitRenderData(
		ParticleRenderer3d& renderer,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) const {
		if (simulator_.GetAliveCount() == 0) {
			return;
		}

		renderer.Submit(simulator_.GetParticles(), config_, emitterTransform, renderLayer);
	}

	bool CpuParticleEmitterRuntime::IsIdle() const {
		return simulator_.GetAliveCount() == 0;
	}

} // MadoEngine::Particle名前空間
