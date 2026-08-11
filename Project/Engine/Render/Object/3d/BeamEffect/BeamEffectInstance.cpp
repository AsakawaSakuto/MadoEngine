#include "BeamEffectInstance.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectRenderer3d.h"
#include <algorithm>
#include <cmath>

namespace {

	/// @brief 2つの色を成分ごとに乗算
	/// @param lhs 左辺色
	/// @param rhs 右辺色
	/// @return 乗算後の色
	Vector4 MultiplyColor(const Vector4& lhs, const Vector4& rhs) {
		return {
			lhs.x * rhs.x,
			lhs.y * rhs.y,
			lhs.z * rhs.z,
			lhs.w * rhs.w,
		};
	}

	/// @brief 長さ方向色Trackへ再生時間による全体色を乗算
	/// @param lengthTrack 始点から終点までの色Track
	/// @param overallColor 再生時間による全体色
	/// @return 全体色を乗算した長さ方向色Track
	MadoEngine::Effect::EffectTrack<Vector4> BuildCombinedColorTrack(
		const MadoEngine::Effect::EffectTrack<Vector4>& lengthTrack,
		const Vector4& overallColor) {
		MadoEngine::Effect::EffectTrack<Vector4> result(
			MultiplyColor(lengthTrack.GetDefaultValue(), overallColor)
		);
		std::vector<MadoEngine::Effect::EffectKeyframe<Vector4>> keys = lengthTrack.GetKeyframes();
		for (MadoEngine::Effect::EffectKeyframe<Vector4>& key : keys) {
			key.value = MultiplyColor(key.value, overallColor);
		}
		result.SetKeyframes(std::move(keys));
		return result;
	}

} // namespace

namespace MadoEngine::Beam {

	void BeamEffectInstance::Initialize(
		std::shared_ptr<const BeamEffectAsset> asset,
		const BeamEffectPlayDesc& desc) {
		asset_ = std::move(asset);
		startPosition_ = desc.startPosition;
		endPosition_ = desc.endPosition;
		sceneType_ = desc.sceneType;
		renderLayer_ = desc.renderLayer;
		playbackSpeed_ = 1.0f;
		isPaused_ = false;
		emitters_.clear();
		if (!asset_) {
			return;
		}
		emitters_.reserve(asset_->GetEmitters().size());
		for (const BeamEmitterConfig& config : asset_->GetEmitters()) {
			if (!config.isEnabled) {
				continue;
			}
			EmitterState state;
			state.config = config;
			state.isLoop = desc.loopOverride.value_or(config.playback.isLoop);
			emitters_.push_back(std::move(state));
		}
	}

	void BeamEffectInstance::Update(float deltaTime) {
		if (isPaused_ || !asset_) {
			return;
		}
		const float safeDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			0.1f
		);

		// 大きなFrame間隔によるTrack補間の飛び越しを抑える時間上限
		const float scaledDeltaTime = safeDeltaTime * playbackSpeed_;
		for (EmitterState& emitter : emitters_) {
			if (emitter.isFinished) {
				continue;
			}
			emitter.totalTime += scaledDeltaTime;
			emitter.playbackTime += scaledDeltaTime;
			const float duration = emitter.config.playback.duration;

			// 停止要求後はLoopへ戻さず現在Cycleの終端で完了
			if (emitter.isLoop && !emitter.isStopping) {
				emitter.playbackTime = std::fmod(emitter.playbackTime, duration);
				continue;
			}
			if (emitter.playbackTime >= duration) {
				emitter.playbackTime = duration;
				emitter.isFinished = true;
			}
		}
	}

	void BeamEffectInstance::Stop(BeamStopMode mode) {
		for (EmitterState& emitter : emitters_) {
			emitter.isLoop = false;
			emitter.isStopping = true;
			if (mode == BeamStopMode::Immediate) {
				emitter.isFinished = true;
			}
		}
	}

	void BeamEffectInstance::Pause() {
		isPaused_ = true;
	}

	void BeamEffectInstance::Resume() {
		isPaused_ = false;
	}

	bool BeamEffectInstance::SetPlaybackSpeed(float playbackSpeed) {
		if (!std::isfinite(playbackSpeed) || playbackSpeed <= 0.0f || playbackSpeed > 256.0f) {
			return false;
		}
		playbackSpeed_ = playbackSpeed;
		return true;
	}

	bool BeamEffectInstance::IsFinished() const {
		return !asset_ || std::all_of(
			emitters_.begin(),
			emitters_.end(),
			[](const EmitterState& emitter) { return emitter.isFinished; }
		);
	}

	bool BeamEffectInstance::Matches(
		SceneType sceneType,
		MadoEngine::Render::RenderLayerMask layerMask) const {
		const bool matchesScene = sceneType_ == SceneType::None || sceneType_ == sceneType;
		return matchesScene && MadoEngine::Render::ContainsRenderLayer(layerMask, renderLayer_);
	}

	void BeamEffectInstance::SubmitRenderData(
		MadoEngine::Ribbon::RibbonEffectRenderer3d& renderer) const {
		if (IsFinished()) {
			return;
		}
		for (const EmitterState& emitter : emitters_) {
			if (emitter.isFinished) {
				continue;
			}
			const BeamEmitterConfig& config = emitter.config;
			const std::vector<MadoEngine::Ribbon::RibbonPoint> points = pointGenerator_.Generate(
				startPosition_,
				endPosition_,
				config.geometry,
				config.noise,
				emitter.totalTime
			);
			if (points.size() < MadoEngine::Ribbon::kMinimumRibbonPointCount) {
				continue;
			}

			const float normalizedTime = std::clamp(
				emitter.playbackTime / config.playback.duration,
				0.0f,
				1.0f
			);
			MadoEngine::Ribbon::RibbonRenderData data;
			data.points = points;
			data.widthOverLifetime.SetDefaultValue((std::max)(
				config.geometry.widthOverTime.Evaluate(normalizedTime),
				0.0f
			));
			const Vector4 overallColor = config.material.colorOverTime.Evaluate(normalizedTime);
			data.colorOverLifetime = BuildCombinedColorTrack(
				config.material.colorOverLength,
				overallColor
			);
			data.interpolation = MadoEngine::Ribbon::RibbonInterpolationMode::Linear;
			data.smoothingSubdivision = 0;
			data.cameraFacing = config.geometry.cameraFacing;
			data.textureName = config.material.textureName;
			data.blendMode = config.material.blendMode;
			data.cullMode = config.material.cullMode;
			data.globalAlpha = std::clamp(
				config.material.globalAlphaOverTime.Evaluate(normalizedTime),
				0.0f,
				1.0f
			);
			data.startAlphaFade = config.geometry.startFade;
			data.endAlphaFade = config.geometry.endFade;
			data.uvScale = config.material.uvScale;
			data.uvOffset = {
				config.material.uvOffset.x + config.material.uvScroll.x * emitter.totalTime,
				config.material.uvOffset.y + config.material.uvScroll.y * emitter.totalTime,
			};
			data.uvMode = config.material.uvMode;
			data.tileLength = config.material.tileLength;
			data.playbackMode = MadoEngine::Ribbon::RibbonPlaybackMode::Reveal;
			data.playbackProgress = std::clamp(
				config.playback.extensionOverTime.Evaluate(normalizedTime),
				0.0f,
				1.0f
			);
			data.renderLayer = renderLayer_;
			renderer.Submit(data);
		}
	}

	void BeamEffectInstance::SetEndpoints(
		const Vector3& startPosition,
		const Vector3& endPosition) {
		startPosition_ = startPosition;
		endPosition_ = endPosition;
	}

	void BeamEffectInstance::SetStartPosition(const Vector3& position) {
		startPosition_ = position;
	}

	void BeamEffectInstance::SetEndPosition(const Vector3& position) {
		endPosition_ = position;
	}

} // namespace MadoEngine::Beam
