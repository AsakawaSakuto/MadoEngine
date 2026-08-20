#include "CylinderEffectInstance.h"
#include "CylinderEffectRenderer3d.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

	using namespace MadoEngine::Effect;

	/// @brief Effect基準TransformへCylinder Emitter位置オフセットを合成
	/// @param config Cylinder Emitter設定
	/// @param effectTransform Effect基準Transform
	/// @return 位置オフセットを合成したEmitter Transform
	Transform3D CreateEmitterTransform(
		const CylinderEmitterConfig& config,
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

	/// @brief 度数法をラジアンへ変換
	/// @param degrees 度数法の角度
	/// @return ラジアン角度
	float ToRadians(float degrees) {
		return degrees * std::numbers::pi_v<float> / 180.0f;
	}

	/// @brief 色を描画可能な範囲へ補正
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
		colorMultiplier_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		playbackSpeed_ = 1.0f;
		isPaused_ = false;
		emitters_.clear();
		if (!asset_) {
			return;
		}
		emitters_.reserve(asset_->GetEmitters().size());
		for (const CylinderEmitterConfig& config : asset_->GetEmitters()) {
			if (!config.isEnabled) {
				continue;
			}
			EmitterState state;
			state.config = config;
			state.isLoop = desc.loopOverride.value_or(config.isLoop);
			emitters_.push_back(std::move(state));
		}
	}

	void CylinderEffectInstance::Update(float deltaTime) {
		if (isPaused_ || !asset_) {
			return;
		}

		// 大きなFrame間隔によるTrack補間の飛び越しを抑える時間上限
		const float scaledDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			0.1f
		) * playbackSpeed_;
		for (EmitterState& emitter : emitters_) {
			if (emitter.isFinished) {
				continue;
			}
			emitter.playbackTime += scaledDeltaTime;
			if (emitter.isLoop) {
				emitter.playbackTime = std::fmod(emitter.playbackTime, emitter.config.duration);
				continue;
			}
			if (emitter.playbackTime >= emitter.config.duration) {
				emitter.playbackTime = emitter.config.duration;
				emitter.isFinished = true;
			}
		}
	}

	void CylinderEffectInstance::Stop(PrimitiveEffectStopMode mode) {
		for (EmitterState& emitter : emitters_) {
			emitter.isLoop = false;
			if (mode == PrimitiveEffectStopMode::Immediate) {
				emitter.isFinished = true;
			}
		}
	}

	void CylinderEffectInstance::Pause() {
		isPaused_ = true;
	}

	void CylinderEffectInstance::Resume() {
		isPaused_ = false;
	}

	bool CylinderEffectInstance::SetPlaybackSpeed(float playbackSpeed) {
		if (!std::isfinite(playbackSpeed) || playbackSpeed <= 0.0f || playbackSpeed > 256.0f) {
			return false;
		}
		playbackSpeed_ = playbackSpeed;
		return true;
	}

	bool CylinderEffectInstance::SetColorMultiplier(const Vector4& colorMultiplier) {
		if (
			!std::isfinite(colorMultiplier.x) ||
			!std::isfinite(colorMultiplier.y) ||
			!std::isfinite(colorMultiplier.z) ||
			!std::isfinite(colorMultiplier.w)) {
			return false;
		}
		colorMultiplier_ = NormalizeColor(colorMultiplier);
		return true;
	}

	bool CylinderEffectInstance::IsFinished() const {
		return !asset_ || std::all_of(
			emitters_.begin(),
			emitters_.end(),
			[](const EmitterState& emitter) { return emitter.isFinished; }
		);
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

		for (const EmitterState& emitter : emitters_) {
			if (emitter.isFinished) {
				continue;
			}

			// 再生時刻に全Trackを評価して不変なAssetとFrame単位の描画Dataを分離
			const CylinderEmitterConfig& config = emitter.config;
			const float playbackTime = emitter.playbackTime;
			CylinderRenderData data;
			data.transform = CreateEmitterTransform(config, transform_);
			data.radialSegments = config.geometry.radialSegments;
			data.heightSegments = config.geometry.heightSegments;
			data.pivot = config.geometry.pivot;
			data.bottomRadii = config.geometry.bottomRadii.Evaluate(playbackTime);
			data.topRadii = config.geometry.topRadii.Evaluate(playbackTime);
			data.bottomRadii.x = (std::max)(0.0f, data.bottomRadii.x);
			data.bottomRadii.y = (std::max)(0.0f, data.bottomRadii.y);
			data.topRadii.x = (std::max)(0.0f, data.topRadii.x);
			data.topRadii.y = (std::max)(0.0f, data.topRadii.y);
			data.height = (std::max)(0.001f, config.geometry.height.Evaluate(playbackTime));
			data.startAngleRadians = ToRadians(config.geometry.startAngleDegrees.Evaluate(playbackTime));
			data.arcAngleRadians = ToRadians(std::clamp(
				config.geometry.arcAngleDegrees.Evaluate(playbackTime),
				-360.0f,
				360.0f
			));
			data.uvDirection = config.material.uv.direction;
			data.uvScale = config.material.uv.scale.Evaluate(playbackTime);
			data.uvOffset = config.material.uv.offset.Evaluate(playbackTime);
			data.uvRotationRadians = ToRadians(config.material.uv.rotationDegrees.Evaluate(playbackTime));
			data.globalAlpha = std::clamp(config.material.globalAlpha.Evaluate(playbackTime), 0.0f, 1.0f);
			data.bottomFadeRange = std::clamp(config.material.bottomFadeRange.Evaluate(playbackTime), 0.0f, 1.0f);
			data.topFadeRange = std::clamp(config.material.topFadeRange.Evaluate(playbackTime), 0.0f, 1.0f);
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
				data.gradient[index].color = NormalizeColor(
					config.material.gradient[index].color.Evaluate(playbackTime) * colorMultiplier_
				);
			}
			renderer.Submit(data);
		}
	}

} // namespace MadoEngine::Effect
