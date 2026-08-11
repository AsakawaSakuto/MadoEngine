#include "RibbonEffectInstance.h"
#include "ManualRibbonPointSource.h"
#include "RibbonEffectRenderer3d.h"
#include "TrailPointSource.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>

namespace MadoEngine::Ribbon {

	void RibbonEffectInstance::Initialize(
		std::shared_ptr<const RibbonEffectAsset> asset,
		const RibbonEffectPlayDesc& desc) {
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
		for (const RibbonEmitterConfig& config : asset_->GetEmitters()) {
			if (!config.isEnabled) {
				continue;
			}
			EmitterState state;
			state.config = config;
			state.isGenerating = true;
			state.isLoop = desc.loopOverride.value_or(config.playback.isLoop);
			if (config.trail.generationMode == RibbonPointGenerationMode::Manual) {
				state.pointSource = std::make_unique<ManualRibbonPointSource>(config.trail, transform_);
			} else {
				state.pointSource = std::make_unique<TrailPointSource>(config.trail, transform_);
			}
			emitters_.push_back(std::move(state));
		}
	}

	void RibbonEffectInstance::Update(float deltaTime) {
		if (isPaused_ || !asset_) {
			return;
		}

		const float safeDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			0.1f
		);

		// 長時間停止後の復帰でTrail Pointが過剰生成されないFrame時間上限
		const float scaledDeltaTime = safeDeltaTime * playbackSpeed_;
		for (EmitterState& emitter : emitters_) {
			if (emitter.isImmediatelyFinished || !emitter.pointSource) {
				continue;
			}
			emitter.pointSource->Update(scaledDeltaTime);
			emitter.totalTime += scaledDeltaTime;
			if (!emitter.isGenerating) {
				continue;
			}

			const float duration = emitter.config.playback.duration;
			emitter.playbackTime += scaledDeltaTime;
			if (emitter.isLoop) {
				emitter.playbackTime = std::fmod(emitter.playbackTime, duration);
				continue;
			}
			if (emitter.playbackTime >= duration) {

				// Finish停止では生成だけを終了して既存Trailの寿命消化を継続
				emitter.playbackTime = duration;
				emitter.isGenerating = false;
				emitter.pointSource->Stop(RibbonStopMode::Finish);
			}
		}
	}

	void RibbonEffectInstance::Stop(RibbonStopMode mode) {
		for (EmitterState& emitter : emitters_) {
			if (!emitter.pointSource || emitter.isImmediatelyFinished) {
				continue;
			}
			emitter.isGenerating = false;
			emitter.isLoop = false;
			emitter.pointSource->Stop(mode);
			if (mode == RibbonStopMode::Immediate) {
				emitter.isImmediatelyFinished = true;
			}
		}
	}

	void RibbonEffectInstance::Pause() {
		isPaused_ = true;
	}

	void RibbonEffectInstance::Resume() {
		isPaused_ = false;
	}

	bool RibbonEffectInstance::SetPlaybackSpeed(float playbackSpeed) {
		if (!std::isfinite(playbackSpeed) || playbackSpeed <= 0.0f || playbackSpeed > 256.0f) {
			return false;
		}
		playbackSpeed_ = playbackSpeed;
		return true;
	}

	bool RibbonEffectInstance::IsFinished() const {
		if (!asset_) {
			return true;
		}
		return std::all_of(
			emitters_.begin(),
			emitters_.end(),
			[](const EmitterState& emitter) {
				return emitter.isImmediatelyFinished || !emitter.pointSource ||
					(!emitter.isGenerating && emitter.pointSource->GetPoints().empty());
			}
		);
	}

	bool RibbonEffectInstance::Matches(
		SceneType sceneType,
		MadoEngine::Render::RenderLayerMask layerMask) const {
		const bool matchesScene = sceneType_ == SceneType::None || sceneType_ == sceneType;
		return matchesScene && MadoEngine::Render::ContainsRenderLayer(layerMask, renderLayer_);
	}

	void RibbonEffectInstance::SubmitRenderData(RibbonEffectRenderer3d& renderer) const {
		if (IsFinished()) {
			return;
		}

		for (const EmitterState& emitter : emitters_) {
			if (emitter.isImmediatelyFinished || !emitter.pointSource ||
				emitter.pointSource->GetPoints().size() < kMinimumRibbonPointCount) {
				continue;
			}
			const RibbonEmitterConfig& config = emitter.config;
			RibbonRenderData data;
			data.points = emitter.pointSource->GetPoints();
			data.widthOverLifetime = config.geometry.widthOverLifetime;
			data.colorOverLifetime = config.material.colorOverLifetime;
			data.interpolation = config.geometry.interpolation;
			data.smoothingSubdivision = config.geometry.smoothingSubdivision;
			data.cameraFacing = config.geometry.cameraFacing;
			data.textureName = config.material.textureName;
			data.blendMode = config.material.blendMode;
			data.cullMode = config.material.cullMode;
			data.globalAlpha = std::clamp(config.material.globalAlpha.Evaluate(emitter.playbackTime), 0.0f, 1.0f);
			data.uvScale = config.material.uvScale;
			data.uvOffset = {
				config.material.uvOffset.x + config.material.uvScroll.x * emitter.totalTime,
				config.material.uvOffset.y + config.material.uvScroll.y * emitter.totalTime,
			};
			data.uvMode = config.material.uvMode;
			data.tileLength = config.material.tileLength;
			const float normalizedPlaybackTime = std::clamp(
				emitter.playbackTime / config.playback.duration,
				0.0f,
				1.0f
			);
			data.playbackMode = config.playback.mode;
			data.playbackProgress = std::clamp(
				config.playback.progress.Evaluate(normalizedPlaybackTime),
				0.0f,
				1.0f
			);
			data.sweepLength = config.playback.sweepLength;
			data.renderLayer = renderLayer_;
			renderer.Submit(data);
		}
	}

	void RibbonEffectInstance::SetTransform(const Transform3D& transform) {
		transform_ = transform;
		for (EmitterState& emitter : emitters_) {
			if (emitter.pointSource) {
				emitter.pointSource->SetTransform(transform_);
			}
		}
	}

	bool RibbonEffectInstance::SetControlPoints(const std::vector<Vector3>& controlPoints) {
		bool wasSet = false;
		for (EmitterState& emitter : emitters_) {
			auto* manualSource = dynamic_cast<ManualRibbonPointSource*>(emitter.pointSource.get());
			if (manualSource) {
				wasSet = manualSource->SetControlPoints(controlPoints) || wasSet;
			}
		}
		return wasSet;
	}

	bool RibbonEffectInstance::SetLocalControlPoints(const std::vector<Vector3>& controlPoints) {
		const Matrix4x4 world = Matrix::MakeAffine(
			transform_.scale,
			transform_.rotate,
			transform_.translate
		);
		bool wasSet = false;
		for (EmitterState& emitter : emitters_) {
			auto* manualSource = dynamic_cast<ManualRibbonPointSource*>(emitter.pointSource.get());
			if (!manualSource) {
				continue;
			}
			if (emitter.config.trail.simulationSpace == RibbonSimulationSpace::Local) {
				wasSet = manualSource->SetControlPoints(controlPoints) || wasSet;
				continue;
			}
			std::vector<Vector3> worldControlPoints = controlPoints;
			for (Vector3& point : worldControlPoints) {
				point = Matrix::Transform(point, world);
			}
			wasSet = manualSource->SetControlPoints(worldControlPoints) || wasSet;
		}
		return wasSet;
	}

	bool RibbonEffectInstance::ClearControlPoints() {
		bool wasCleared = false;
		for (EmitterState& emitter : emitters_) {
			auto* manualSource = dynamic_cast<ManualRibbonPointSource*>(emitter.pointSource.get());
			if (manualSource) {
				manualSource->Clear();
				wasCleared = true;
			}
		}
		return wasCleared;
	}

} // namespace MadoEngine::Ribbon
