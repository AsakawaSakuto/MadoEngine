#include "ParticleEffectInstance.h"
#include "Math/Function/MatrixFunction.h"
#include "ParticleRenderer3d.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectRenderer3d.h"
#include "Utility/Random.h"
#include <algorithm>
#include <cmath>

namespace {

	using namespace MadoEngine::Particle;

	/// @brief Effect基準TransformへEmitter位置オフセットを合成
	/// @param config Emitter設定
	/// @param effectTransform Effect基準Transform
	/// @return 位置オフセットを合成したEmitter Transform
	Transform3D CreateEmitterTransform(
		const EmitterConfig& config,
		const Transform3D& effectTransform) {
		Transform3D emitterTransform = effectTransform;
		const Matrix4x4 effectMatrix = Matrix::MakeAffine(
			effectTransform.scale,
			effectTransform.rotate,
			effectTransform.translate
		);
		emitterTransform.translate = Matrix::Transform(config.translateOffset, effectMatrix);
		return emitterTransform;
	}

} // namespace

namespace MadoEngine::Particle {

	void ParticleEmitterInstance::Initialize(
		const EmitterConfig& config,
		uint32_t randomSeed,
		const std::optional<bool>& loopOverride,
		const Transform3D& emitterTransform,
		const std::optional<ParticleBackend>& backendOverride,
		const ParticleEmitterRuntimeFactory& runtimeFactory) {
		config_ = config;
		requestedBackend_ = backendOverride.value_or(config.backend);
		if (requestedBackend_ == ParticleBackend::Count) {
			requestedBackend_ = ParticleBackend::Auto;
		}
		runtime_ = runtimeFactory.Create(
			config,
			randomSeed,
			backendOverride,
			fallbackReason_
		);
		isLoop_ = loopOverride.value_or(config.emission.isLoop);
		Reset();

		if (config.emission.startDelay <= 0.0f) {
			EmitBursts(0.0f, 0.0f, CreateEmitterTransform(config, emitterTransform));
			hasProcessedEmission_ = true;
		}
	}

	void ParticleEmitterInstance::Update(float deltaTime, const Transform3D& emitterTransform) {
		if (!config_ || deltaTime <= 0.0f) {
			return;
		}

		if (!runtime_) {
			isEmitting_ = false;
			return;
		}

		const Transform3D offsetEmitterTransform = CreateEmitterTransform(*config_, emitterTransform);
		runtime_->Update(deltaTime, offsetEmitterTransform);
		if (!isEmitting_) {
			return;
		}

		const float previousTime = playbackTime_;
		playbackTime_ += deltaTime;
		const float delay = config_->emission.startDelay;
		const float previousLocalTime = previousTime - delay;
		const float currentLocalTime = playbackTime_ - delay;

		// Start Delay前の時間をEmission計算へ含めない待機区間
		if (currentLocalTime < 0.0f) {
			return;
		}

		float emissionDeltaTime = 0.0f;

		// 非Loop時はDuration外のFrame時間を除いて総生成数の超過を防止
		if (isLoop_) {
			emissionDeltaTime = (std::max)(0.0f, currentLocalTime) - (std::max)(0.0f, previousLocalTime);
		} else {
			const float duration = config_->emission.duration;
			const float previousClamped = std::clamp(previousLocalTime, 0.0f, duration);
			const float currentClamped = std::clamp(currentLocalTime, 0.0f, duration);
			emissionDeltaTime = (std::max)(0.0f, currentClamped - previousClamped);
		}

		// 小数Particleを次Frameへ持ち越してFrame Rate非依存の連続生成数を維持
		spawnAccumulator_ += config_->emission.ratePerSecond * emissionDeltaTime;
		const uint32_t continuousSpawnCount = static_cast<uint32_t>(spawnAccumulator_);
		spawnAccumulator_ -= static_cast<float>(continuousSpawnCount);
		if (continuousSpawnCount > 0) {
			runtime_->Emit(continuousSpawnCount, offsetEmitterTransform);
		}

		EmitBursts(previousLocalTime, currentLocalTime, offsetEmitterTransform);
		hasProcessedEmission_ = true;

		if (!isLoop_ && currentLocalTime >= config_->emission.duration) {

			// 生成終了後も既存Particleが消滅するまではInstanceを存続
			isEmitting_ = false;
		}
	}

	void ParticleEmitterInstance::Stop(StopMode mode) {
		isEmitting_ = false;
		if (runtime_) {
			runtime_->Stop(mode);
		}
	}

	void ParticleEmitterInstance::SetTransform(const Transform3D& emitterTransform) {
		if (config_ && runtime_) {
			runtime_->SetTransform(CreateEmitterTransform(*config_, emitterTransform));
		}
	}

	void ParticleEmitterInstance::Reset() {
		playbackTime_ = 0.0f;
		spawnAccumulator_ = 0.0f;
		isEmitting_ = true;
		hasProcessedEmission_ = false;
		if (runtime_) {
			runtime_->Reset();
		}
	}

	void ParticleEmitterInstance::RecordGpuSimulation(
		ID3D12GraphicsCommandList* commandList,
		uint64_t submissionFenceValue) {
		if (runtime_) {
			runtime_->RecordGpuSimulation(commandList, submissionFenceValue);
		}
	}

	void ParticleEmitterInstance::OnGpuFrameCompleted(uint64_t completedFenceValue) {
		if (runtime_) {
			runtime_->OnGpuFrameCompleted(completedFenceValue);
		}
	}

	bool ParticleEmitterInstance::IsFinished() const {
		return !isEmitting_ && (!runtime_ || runtime_->IsIdle());
	}

	void ParticleEmitterInstance::SubmitRenderData(
		ParticleRenderer3d& renderer,
		MadoEngine::Ribbon::RibbonEffectRenderer3d& trailRenderer,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) const {
		if (!config_ || !runtime_) {
			return;
		}

		const Transform3D resolvedEmitterTransform = CreateEmitterTransform(*config_, emitterTransform);
		runtime_->SubmitRenderData(
			renderer,
			resolvedEmitterTransform,
			renderLayer
		);
		runtime_->SubmitTrailRenderData(
			trailRenderer,
			resolvedEmitterTransform,
			renderLayer
		);
	}

	ParticleEmitterRuntimeInfo ParticleEmitterInstance::GetRuntimeInfo() const {
		ParticleEmitterRuntimeInfo info;
		if (config_) {
			info.name = config_->name;
			info.maxParticleCount = config_->emission.maxParticles;
		}
		info.requestedBackend = requestedBackend_;
		info.fallbackReason = fallbackReason_;
		if (runtime_) {
			info.activeBackend = runtime_->GetBackend();
			info.aliveParticleCount = runtime_->GetAliveCount();
			info.maxParticleCount = runtime_->GetMaxParticleCount();
			info.gpuBufferCapacityBytes = runtime_->GetGpuBufferCapacityBytes();
		}
		return info;
	}

	void ParticleEmitterInstance::EmitBursts(
		float previousLocalTime,
		float currentLocalTime,
		const Transform3D& emitterTransform) {
		if (!config_ || currentLocalTime < 0.0f) {
			return;
		}

		const float clampedPreviousTime = (std::max)(0.0f, previousLocalTime);
		for (const BurstConfig& burst : config_->emission.bursts) {
			if (!isLoop_ || config_->emission.duration <= 0.0f) {
				const bool isInitialBurst = !hasProcessedEmission_ && burst.time <= 0.0f;
				const bool crossedBurst = burst.time > clampedPreviousTime && burst.time <= currentLocalTime;
				if (isInitialBurst || crossedBurst) {
					if (runtime_) {
						runtime_->Emit(burst.count, emitterTransform);
					}
				}
				continue;
			}

			const float duration = config_->emission.duration;
			int firstCycle = static_cast<int>(std::floor((clampedPreviousTime - burst.time) / duration)) + 1;
			firstCycle = (std::max)(0, firstCycle);
			if (!hasProcessedEmission_ && burst.time <= 0.0f) {
				firstCycle = 0;
			}
			const int lastCycle = static_cast<int>(std::floor((currentLocalTime - burst.time) / duration));
			for (int cycle = firstCycle; cycle <= lastCycle; ++cycle) {
				const float eventTime = static_cast<float>(cycle) * duration + burst.time;
				if (eventTime < 0.0f || eventTime > currentLocalTime) {
					continue;
				}
				if (runtime_) {
					runtime_->Emit(burst.count, emitterTransform);
				}
			}
		}
	}

	void ParticleEffectInstance::Initialize(
		std::shared_ptr<const ParticleEffectAsset> asset,
		const PlayDesc& desc,
		const ParticleEmitterRuntimeFactory& runtimeFactory) {
		asset_ = std::move(asset);
		transform_ = desc.transform;
		sceneType_ = desc.sceneType;
		renderLayer_ = desc.renderLayer;
		playbackSpeed_ = 1.0f;
		isPaused_ = false;
		emitters_.clear();

		if (!asset_) {
			return;
		}

		emitters_.reserve(asset_->GetEmitters().size());
		for (std::size_t index = 0; index < asset_->GetEmitters().size(); ++index) {
			const EmitterConfig& config = asset_->GetEmitters()[index];
			if (!config.isEnabled) {
				continue;
			}
			const uint32_t emitterSeed = MyRand::MakeDerivedSeed(desc.randomSeed, static_cast<uint32_t>(index));
			ParticleEmitterInstance& emitter = emitters_.emplace_back();
			emitter.Initialize(
				config,
				emitterSeed,
				desc.loopOverride,
				desc.transform,
				desc.backendOverride,
				runtimeFactory
			);
		}
	}

	void ParticleEffectInstance::Update(float deltaTime) {
		if (isPaused_) {
			return;
		}

		const float scaledDeltaTime = deltaTime * playbackSpeed_;
		for (ParticleEmitterInstance& emitter : emitters_) {
			emitter.Update(scaledDeltaTime, transform_);
		}
	}

	void ParticleEffectInstance::RecordGpuSimulation(
		ID3D12GraphicsCommandList* commandList,
		uint64_t submissionFenceValue) {
		for (ParticleEmitterInstance& emitter : emitters_) {
			emitter.RecordGpuSimulation(commandList, submissionFenceValue);
		}
	}

	void ParticleEffectInstance::OnGpuFrameCompleted(uint64_t completedFenceValue) {
		for (ParticleEmitterInstance& emitter : emitters_) {
			emitter.OnGpuFrameCompleted(completedFenceValue);
		}
	}

	void ParticleEffectInstance::Stop(StopMode mode) {
		for (ParticleEmitterInstance& emitter : emitters_) {
			emitter.Stop(mode);
		}
	}

	void ParticleEffectInstance::Pause() {
		isPaused_ = true;
	}

	void ParticleEffectInstance::Resume() {
		isPaused_ = false;
	}

	bool ParticleEffectInstance::SetPlaybackSpeed(float playbackSpeed) {
		if (!std::isfinite(playbackSpeed) || playbackSpeed <= 0.0f || playbackSpeed > 256.0f) {
			return false;
		}
		playbackSpeed_ = playbackSpeed;
		return true;
	}

	void ParticleEffectInstance::SetTransform(const Transform3D& transform) {
		transform_ = transform;
		for (ParticleEmitterInstance& emitter : emitters_) {
			emitter.SetTransform(transform_);
		}
	}

	bool ParticleEffectInstance::IsFinished() const {
		return std::all_of(emitters_.begin(), emitters_.end(), [](const ParticleEmitterInstance& emitter) {
			return emitter.IsFinished();
		});
	}

	bool ParticleEffectInstance::Matches(
		SceneType sceneType,
		MadoEngine::Render::RenderLayerMask layerMask) const {
		const bool matchesScene = sceneType_ == SceneType::None || sceneType_ == sceneType;
		return matchesScene && MadoEngine::Render::ContainsRenderLayer(layerMask, renderLayer_);
	}

	void ParticleEffectInstance::SubmitRenderData(
		ParticleRenderer3d& renderer,
		MadoEngine::Ribbon::RibbonEffectRenderer3d& trailRenderer) const {
		for (const ParticleEmitterInstance& emitter : emitters_) {
			emitter.SubmitRenderData(renderer, trailRenderer, transform_, renderLayer_);
		}
	}

	std::size_t ParticleEffectInstance::GetAliveCount() const {
		std::size_t count = 0;
		for (const ParticleEmitterInstance& emitter : emitters_) {
			count += emitter.GetAliveCount();
		}
		return count;
	}

	std::vector<ParticleEmitterRuntimeInfo> ParticleEffectInstance::GetRuntimeInfo() const {
		std::vector<ParticleEmitterRuntimeInfo> runtimeInfo;
		runtimeInfo.reserve(emitters_.size());
		for (const ParticleEmitterInstance& emitter : emitters_) {
			runtimeInfo.push_back(emitter.GetRuntimeInfo());
		}
		return runtimeInfo;
	}

} // namespace MadoEngine::Particle
