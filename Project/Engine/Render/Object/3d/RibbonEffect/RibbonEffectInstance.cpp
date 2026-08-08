#include "RibbonEffectInstance.h"
#include "ManualRibbonPointSource.h"
#include "RibbonEffectRenderer3d.h"
#include "TrailPointSource.h"
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
		playbackTime_ = 0.0f;
		totalTime_ = 0.0f;
		playbackSpeed_ = 1.0f;
		isGenerating_ = asset_ != nullptr;
		isImmediatelyFinished_ = asset_ == nullptr;
		isPaused_ = false;
		isLoop_ = asset_ ? asset_->GetConfig().playback.isLoop : false;
		if (desc.loopOverride.has_value()) {
			isLoop_ = desc.loopOverride.value();
		}

		if (!asset_) {
			return;
		}
		const RibbonTrailModule& trail = asset_->GetConfig().trail;
		if (trail.generationMode == RibbonPointGenerationMode::Manual) {
			pointSource_ = std::make_unique<ManualRibbonPointSource>(trail, transform_);
		} else {
			pointSource_ = std::make_unique<TrailPointSource>(trail, transform_);
		}
	}

	void RibbonEffectInstance::Update(float deltaTime) {
		if (isImmediatelyFinished_ || isPaused_ || !asset_ || !pointSource_) {
			return;
		}

		const float safeDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			0.1f
		);
		const float scaledDeltaTime = safeDeltaTime * playbackSpeed_;
		pointSource_->Update(scaledDeltaTime);
		totalTime_ += scaledDeltaTime;
		if (!isGenerating_) {
			return;
		}

		const float duration = asset_->GetConfig().playback.duration;
		playbackTime_ += scaledDeltaTime;
		if (isLoop_) {
			playbackTime_ = std::fmod(playbackTime_, duration);
			return;
		}
		if (playbackTime_ >= duration) {
			playbackTime_ = duration;
			Stop(RibbonStopMode::Finish);
		}
	}

	void RibbonEffectInstance::Stop(RibbonStopMode mode) {
		if (!pointSource_ || isImmediatelyFinished_) {
			return;
		}
		isGenerating_ = false;
		isLoop_ = false;
		pointSource_->Stop(mode);
		if (mode == RibbonStopMode::Immediate) {
			isImmediatelyFinished_ = true;
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
		if (isImmediatelyFinished_ || !asset_ || !pointSource_) {
			return true;
		}
		return !isGenerating_ && pointSource_->GetPoints().empty();
	}

	bool RibbonEffectInstance::Matches(
		SceneType sceneType,
		MadoEngine::Render::RenderLayerMask layerMask) const {
		const bool matchesScene = sceneType_ == SceneType::None || sceneType_ == sceneType;
		return matchesScene && MadoEngine::Render::ContainsRenderLayer(layerMask, renderLayer_);
	}

	void RibbonEffectInstance::SubmitRenderData(RibbonEffectRenderer3d& renderer) const {
		if (IsFinished() || pointSource_->GetPoints().size() < kMinimumRibbonPointCount) {
			return;
		}

		const RibbonEffectConfig& config = asset_->GetConfig();
		RibbonRenderData data;
		data.points = pointSource_->GetPoints();
		data.widthOverLifetime = config.geometry.widthOverLifetime;
		data.colorOverLifetime = config.material.colorOverLifetime;
		data.interpolation = config.geometry.interpolation;
		data.smoothingSubdivision = config.geometry.smoothingSubdivision;
		data.cameraFacing = config.geometry.cameraFacing;
		data.textureName = config.material.textureName;
		data.blendMode = config.material.blendMode;
		data.cullMode = config.material.cullMode;
		data.globalAlpha = std::clamp(config.material.globalAlpha.Evaluate(playbackTime_), 0.0f, 1.0f);
		data.uvScale = config.material.uvScale;
		data.uvOffset = {
			config.material.uvOffset.x + config.material.uvScroll.x * totalTime_,
			config.material.uvOffset.y + config.material.uvScroll.y * totalTime_,
		};
		data.uvMode = config.material.uvMode;
		data.tileLength = config.material.tileLength;
		const float normalizedPlaybackTime = std::clamp(
			playbackTime_ / config.playback.duration,
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

	void RibbonEffectInstance::SetTransform(const Transform3D& transform) {
		transform_ = transform;
		if (pointSource_) {
			pointSource_->SetTransform(transform_);
		}
	}

	bool RibbonEffectInstance::SetControlPoints(const std::vector<Vector3>& controlPoints) {
		auto* manualSource = dynamic_cast<ManualRibbonPointSource*>(pointSource_.get());
		return manualSource && manualSource->SetControlPoints(controlPoints);
	}

	bool RibbonEffectInstance::ClearControlPoints() {
		auto* manualSource = dynamic_cast<ManualRibbonPointSource*>(pointSource_.get());
		if (!manualSource) {
			return false;
		}
		manualSource->Clear();
		return true;
	}

} // namespace MadoEngine::Ribbon
