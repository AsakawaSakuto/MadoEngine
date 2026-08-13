#include "ParticleTrailHistory.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectRenderer3d.h"
#include <algorithm>
#include <cmath>

namespace {

	constexpr float kParticleTrailPositionEpsilon = 0.000001f;

	/// @brief Vector3の全要素が有限値か確認
	/// @param value 確認対象
	/// @return 全要素が有限値の場合はtrue
	bool IsFiniteVector3(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	/// @brief Trail幅の寿命Trackを生成
	/// @param startValue 発生直後の値
	/// @param endValue 消滅直前の値
	/// @return 生成した寿命Track
	MadoEngine::Effect::EffectTrack<float> CreateTrailWidthTrack(
		float startValue,
		float endValue) {
		MadoEngine::Effect::EffectTrack<float> track(startValue);
		track.SetKeyframes({
			{ 0.0f, startValue, EaseType::Linear },
			{ 1.0f, endValue, EaseType::Linear },
		});
		return track;
	}

	/// @brief Trail色の寿命Trackを生成
	/// @param startValue 発生直後の値
	/// @param endValue 消滅直前の値
	/// @return 生成した寿命Track
	MadoEngine::Effect::EffectTrack<Vector4> CreateTrailColorTrack(
		const Vector4& startValue,
		const Vector4& endValue) {
		MadoEngine::Effect::EffectTrack<Vector4> track(startValue);
		track.SetKeyframes({
			{ 0.0f, startValue, EaseType::Linear },
			{ 1.0f, endValue, EaseType::Linear },
		});
		return track;
	}

} // namespace

namespace MadoEngine::Particle {

	void ParticleTrailHistory::Initialize(
		const ParticleTrailModule& trail,
		SimulationSpace simulationSpace) {
		config_ = trail;
		simulationSpace_ = simulationSpace;
		Clear();
	}

	void ParticleTrailHistory::Advance(float deltaTime) {
		if (!config_.isEnabled) {
			Clear();
			return;
		}

		const float safeDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			0.1f
		);
		for (auto& [identifier, state] : trails_) {
			(void)identifier;
			for (TrailPoint& point : state.points) {
				point.age += safeDeltaTime;
			}
			std::erase_if(state.points, [this](const TrailPoint& point) {
				return point.age >= config_.pointLifetime;
			});
		}
		std::erase_if(trails_, [](const auto& entry) {
			const TrailState& state = entry.second;
			return !state.isParticleAlive && state.points.empty();
		});
	}

	void ParticleTrailHistory::UpdateParticles(
		std::span<const ParticleTrailSample> samples) {
		if (!config_.isEnabled) {
			Clear();
			return;
		}

		for (auto& [identifier, state] : trails_) {
			(void)identifier;
			state.wasParticleAlive = state.isParticleAlive;
			state.isParticleAlive = false;
		}

		// Backend固有の格納順に依存せず安定IDから粒子固有の履歴へ位置を反映
		for (const ParticleTrailSample& sample : samples) {
			TrailState& state = trails_[sample.identifier];
			state.isParticleAlive = true;
			state.latestPosition = sample.position;
			state.latestColor = sample.color;
			state.hasLatestPosition = true;
			TryAddPoint(state, sample.position, sample.color);
		}

		for (auto& [identifier, state] : trails_) {
			(void)identifier;
			if (state.wasParticleAlive && !state.isParticleAlive && state.hasLatestPosition) {

				// 粒子消滅時の最終位置を固定し、残存Trailだけを寿命まで減衰
				TryAddPoint(state, state.latestPosition, state.latestColor);
			}
		}
		std::erase_if(trails_, [](const auto& entry) {
			const TrailState& state = entry.second;
			return !state.isParticleAlive && state.points.empty();
		});
	}

	void ParticleTrailHistory::Clear() {
		trails_.clear();
	}

	void ParticleTrailHistory::SubmitRenderData(
		MadoEngine::Ribbon::RibbonEffectRenderer3d& renderer,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) const {
		if (!config_.isEnabled || trails_.empty()) {
			return;
		}

		const Matrix4x4 emitterMatrix = Matrix::MakeAffine(
			emitterTransform.scale,
			emitterTransform.rotate,
			emitterTransform.translate
		);
		const MadoEngine::Effect::EffectTrack<float> widthTrack = CreateTrailWidthTrack(
			config_.startWidth,
			config_.endWidth
		);
		const MadoEngine::Effect::EffectTrack<Vector4> colorTrack = CreateTrailColorTrack(
			config_.startColor,
			config_.endColor
		);

		for (const auto& [identifier, state] : trails_) {
			(void)identifier;
			if (state.points.empty()) {
				continue;
			}

			const TrailPoint& lastPoint = state.points.back();
			const bool appendLatestPosition =
				state.isParticleAlive &&
				state.hasLatestPosition &&
				(state.latestPosition - lastPoint.position).LengthSq() > kParticleTrailPositionEpsilon;
			const std::size_t firstPointIndex =
				appendLatestPosition && state.points.size() >= config_.maxPointCount
				? 1
				: 0;

			MadoEngine::Ribbon::RibbonRenderData renderData;
			renderData.points.reserve(
				state.points.size() - firstPointIndex + (appendLatestPosition ? 1u : 0u)
			);
			for (std::size_t pointIndex = firstPointIndex; pointIndex < state.points.size(); ++pointIndex) {
				const TrailPoint& point = state.points[pointIndex];
				renderData.points.push_back({
					simulationSpace_ == SimulationSpace::Local
						? Matrix::Transform(point.position, emitterMatrix)
						: point.position,
					point.age,
					config_.pointLifetime,
					config_.syncParticleColor ? point.color : Vector4{ 1.0f, 1.0f, 1.0f, 1.0f },
				});
			}
			if (appendLatestPosition) {

				// 最小間隔未満でも描画時の先端だけは現在位置へ追従させて軌跡の遅延を抑制
				renderData.points.push_back({
					simulationSpace_ == SimulationSpace::Local
						? Matrix::Transform(state.latestPosition, emitterMatrix)
						: state.latestPosition,
					0.0f,
					config_.pointLifetime,
					config_.syncParticleColor ? state.latestColor : Vector4{ 1.0f, 1.0f, 1.0f, 1.0f },
				});
			}
			if (renderData.points.size() < kMinimumParticleTrailPointCount) {
				continue;
			}

			renderData.widthOverLifetime = widthTrack;
			renderData.colorOverLifetime = colorTrack;
			renderData.interpolation = config_.interpolation == ParticleTrailInterpolation::CatmullRom
				? MadoEngine::Ribbon::RibbonInterpolationMode::CatmullRom
				: MadoEngine::Ribbon::RibbonInterpolationMode::Linear;
			renderData.smoothingSubdivision = config_.smoothingSubdivision;
			renderData.cameraFacing = config_.cameraFacing;
			renderData.textureName = config_.textureName;
			renderData.blendMode = config_.blendMode;
			renderData.cullMode = MadoEngine::Render::CullMode::None;
			renderData.renderLayer = renderLayer;
			renderer.Submit(renderData);
		}
	}

	void ParticleTrailHistory::TryAddPoint(
		TrailState& state,
		const Vector3& position,
		const Vector4& color) {
		if (!IsFiniteVector3(position)) {
			return;
		}

		const float minimumDistance = (std::max)(
			config_.minPointDistance,
			kParticleTrailPositionEpsilon
		);
		if (!state.points.empty()) {
			const Vector3 difference = position - state.points.back().position;
			if (difference.LengthSq() < minimumDistance * minimumDistance) {

				// 微小移動によるPoint過密化とRibbon Meshの不要な増加を抑制
				return;
			}
		}

		state.points.push_back({ position, 0.0f, color });
		while (state.points.size() > config_.maxPointCount) {
			state.points.erase(state.points.begin());
		}
	}

} // namespace MadoEngine::Particle
