#include "CylinderEffectInstance.h"
#include "CylinderEffectRenderer3d.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

	/// @brief 度数法をラジアンへ変換する
	/// @param degrees 度数法の角度
	/// @return ラジアン角度
	float ToRadians(float degrees) {
		return degrees * std::numbers::pi_v<float> / 180.0f;
	}

	/// @brief 色を描画可能な範囲へ補正する
	/// @param color 補正する色
	/// @return 補正後の色
	Vector4 NormalizeColor(const Vector4& color) {
		return {
			(std::max)(0.0f, color.x),
			(std::max)(0.0f, color.y),
			(std::max)(0.0f, color.z),
			std::clamp(color.w, 0.0f, 1.0f),
		};
	}

} // namespace

namespace MadoEngine::Effect {

	void CylinderEffectInstance::Initialize(
		std::shared_ptr<const CylinderEffectAsset> asset,
		const PrimitiveEffectPlayDesc& desc) {
		asset_ = std::move(asset);
		transform_ = desc.transform;
		sceneType_ = desc.sceneType;
		renderLayer_ = desc.renderLayer;
		playbackTime_ = 0.0f;
		isFinished_ = false;
		isLoop_ = asset_ ? asset_->GetConfig().isLoop : false;
		if (desc.loopOverride.has_value()) {
			isLoop_ = desc.loopOverride.value();
		}
	}

	void CylinderEffectInstance::Update(float deltaTime) {
		if (isFinished_ || !asset_) {
			return;
		}

		const float duration = asset_->GetConfig().duration;
		playbackTime_ += std::clamp(deltaTime, 0.0f, 0.1f);
		if (isLoop_) {
			playbackTime_ = std::fmod(playbackTime_, duration);
			return;
		}

		if (playbackTime_ >= duration) {
			playbackTime_ = duration;
			isFinished_ = true;
		}
	}

	void CylinderEffectInstance::Stop(PrimitiveEffectStopMode mode) {
		if (mode == PrimitiveEffectStopMode::Immediate) {
			isFinished_ = true;
			return;
		}
		isLoop_ = false;
	}

	bool CylinderEffectInstance::IsFinished() const {
		return isFinished_ || !asset_;
	}

	bool CylinderEffectInstance::Matches(
		SceneType sceneType,
		MadoEngine::Render::RenderLayerMask layerMask) const {
		const bool matchesScene = sceneType_ == SceneType::None || sceneType_ == sceneType;
		return matchesScene && MadoEngine::Render::ContainsRenderLayer(layerMask, renderLayer_);
	}

	void CylinderEffectInstance::SubmitRenderData(CylinderEffectRenderer3d& renderer) const {
		if (IsFinished()) {
			return;
		}

		const CylinderEffectConfig& config = asset_->GetConfig();
		CylinderRenderData data;
		data.transform = transform_;
		data.radialSegments = config.geometry.radialSegments;
		data.heightSegments = config.geometry.heightSegments;
		data.pivot = config.geometry.pivot;
		data.bottomRadii = config.geometry.bottomRadii.Evaluate(playbackTime_);
		data.topRadii = config.geometry.topRadii.Evaluate(playbackTime_);
		data.bottomRadii.x = (std::max)(0.0f, data.bottomRadii.x);
		data.bottomRadii.y = (std::max)(0.0f, data.bottomRadii.y);
		data.topRadii.x = (std::max)(0.0f, data.topRadii.x);
		data.topRadii.y = (std::max)(0.0f, data.topRadii.y);
		data.height = (std::max)(0.001f, config.geometry.height.Evaluate(playbackTime_));
		data.startAngleRadians = ToRadians(config.geometry.startAngleDegrees.Evaluate(playbackTime_));
		data.arcAngleRadians = ToRadians(std::clamp(
			config.geometry.arcAngleDegrees.Evaluate(playbackTime_),
			-360.0f,
			360.0f
		));
		data.uvDirection = config.material.uv.direction;
		data.uvScale = config.material.uv.scale.Evaluate(playbackTime_);
		data.uvOffset = config.material.uv.offset.Evaluate(playbackTime_);
		data.uvRotationRadians = ToRadians(config.material.uv.rotationDegrees.Evaluate(playbackTime_));
		data.globalAlpha = std::clamp(config.material.globalAlpha.Evaluate(playbackTime_), 0.0f, 1.0f);
		data.bottomFadeRange = std::clamp(config.material.bottomFadeRange.Evaluate(playbackTime_), 0.0f, 1.0f);
		data.topFadeRange = std::clamp(config.material.topFadeRange.Evaluate(playbackTime_), 0.0f, 1.0f);
		data.textureName = config.material.textureName;
		data.blendMode = config.material.blendMode;
		data.cullMode = config.material.cullMode;
		data.renderLayer = renderLayer_;

		data.gradientCount = static_cast<uint32_t>((std::min)(
			config.material.gradient.size(),
			static_cast<std::size_t>(kMaximumCylinderGradientStops)
		));
		for (uint32_t index = 0; index < data.gradientCount; ++index) {
			data.gradient[index].position = config.material.gradient[index].position;
			data.gradient[index].color = NormalizeColor(config.material.gradient[index].color.Evaluate(playbackTime_));
		}
		renderer.Submit(data);
	}

} // namespace MadoEngine::Effect
