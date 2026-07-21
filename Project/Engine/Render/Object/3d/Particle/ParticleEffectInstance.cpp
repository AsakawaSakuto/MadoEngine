#include "ParticleEffectInstance.h"
#include "ParticleRenderer3d.h"
#include <algorithm>
#include <cmath>

namespace MadoEngine::Particle {

	void ParticleEmitterInstance::Initialize(
		const EmitterConfig& config,
		uint32_t randomSeed,
		const std::optional<bool>& loopOverride,
		const Transform3D& emitterTransform) {
		config_ = config;
		random_.SetSeed(randomSeed);
		isLoop_ = loopOverride.value_or(config.emission.isLoop);
		simulator_.Initialize(config.emission.maxParticles);
		Reset();

		if (config.emission.startDelay <= 0.0f) {
			EmitBursts(0.0f, 0.0f, emitterTransform);
			hasProcessedEmission_ = true;
		}
	}

	void ParticleEmitterInstance::Update(float deltaTime, const Transform3D& emitterTransform) {
		if (!config_ || deltaTime <= 0.0f) {
			return;
		}

		simulator_.Update(deltaTime, *config_);
		if (!isEmitting_) {
			return;
		}

		const float previousTime = playbackTime_;
		playbackTime_ += deltaTime;
		const float delay = config_->emission.startDelay;
		const float previousLocalTime = previousTime - delay;
		const float currentLocalTime = playbackTime_ - delay;
		if (currentLocalTime < 0.0f) {
			return;
		}

		float emissionDeltaTime = 0.0f;
		if (isLoop_) {
			emissionDeltaTime = (std::max)(0.0f, currentLocalTime) - (std::max)(0.0f, previousLocalTime);
		} else {
			const float duration = config_->emission.duration;
			const float previousClamped = std::clamp(previousLocalTime, 0.0f, duration);
			const float currentClamped = std::clamp(currentLocalTime, 0.0f, duration);
			emissionDeltaTime = (std::max)(0.0f, currentClamped - previousClamped);
		}

		spawnAccumulator_ += config_->emission.ratePerSecond * emissionDeltaTime;
		const uint32_t continuousSpawnCount = static_cast<uint32_t>(spawnAccumulator_);
		spawnAccumulator_ -= static_cast<float>(continuousSpawnCount);
		if (continuousSpawnCount > 0) {
			simulator_.Emit(*config_, emitterTransform, continuousSpawnCount, random_);
		}

		EmitBursts(previousLocalTime, currentLocalTime, emitterTransform);
		hasProcessedEmission_ = true;

		if (!isLoop_ && currentLocalTime >= config_->emission.duration) {
			isEmitting_ = false;
		}
	}

	void ParticleEmitterInstance::Stop(StopMode mode) {
		isEmitting_ = false;
		if (mode == StopMode::Immediate) {
			simulator_.Reset();
		}
	}

	void ParticleEmitterInstance::Reset() {
		playbackTime_ = 0.0f;
		spawnAccumulator_ = 0.0f;
		isEmitting_ = true;
		hasProcessedEmission_ = false;
		simulator_.Reset();
	}

	bool ParticleEmitterInstance::IsFinished() const {
		return !isEmitting_ && simulator_.GetAliveCount() == 0;
	}

	void ParticleEmitterInstance::SubmitRenderData(
		ParticleRenderer3d& renderer,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) const {
		if (!config_ || simulator_.GetAliveCount() == 0) {
			return;
		}

		renderer.Submit(simulator_.GetParticles(), *config_, emitterTransform, renderLayer);
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
					simulator_.Emit(*config_, emitterTransform, burst.count, random_);
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
				simulator_.Emit(*config_, emitterTransform, burst.count, random_);
			}
		}
	}

	void ParticleEffectInstance::Initialize(
		std::shared_ptr<const ParticleEffectAsset> asset,
		const PlayDesc& desc) {
		asset_ = std::move(asset);
		transform_ = desc.transform;
		sceneType_ = desc.sceneType;
		renderLayer_ = desc.renderLayer;
		emitters_.clear();

		if (!asset_) {
			return;
		}

		emitters_.resize(asset_->GetEmitters().size());
		for (std::size_t index = 0; index < emitters_.size(); ++index) {
			const uint32_t emitterSeed = MyRand::MakeDerivedSeed(desc.randomSeed, static_cast<uint32_t>(index));
			emitters_[index].Initialize(
				asset_->GetEmitters()[index],
				emitterSeed,
				desc.loopOverride,
				desc.transform
			);
		}
	}

	void ParticleEffectInstance::Update(float deltaTime) {
		for (ParticleEmitterInstance& emitter : emitters_) {
			emitter.Update(deltaTime, transform_);
		}
	}

	void ParticleEffectInstance::Stop(StopMode mode) {
		for (ParticleEmitterInstance& emitter : emitters_) {
			emitter.Stop(mode);
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

	void ParticleEffectInstance::SubmitRenderData(ParticleRenderer3d& renderer) const {
		for (const ParticleEmitterInstance& emitter : emitters_) {
			emitter.SubmitRenderData(renderer, transform_, renderLayer_);
		}
	}

	std::size_t ParticleEffectInstance::GetAliveCount() const {
		std::size_t count = 0;
		for (const ParticleEmitterInstance& emitter : emitters_) {
			count += emitter.GetAliveCount();
		}
		return count;
	}

} // namespace MadoEngine::Particle
