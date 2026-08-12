#include "CpuParticleEmitterRuntime.h"
#include "Math/Function/MatrixFunction.h"
#include "ParticleRenderer3d.h"
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

	bool CpuParticleEmitterRuntime::Initialize(const EmitterConfig& config, uint32_t randomSeed) {
		config_ = config;
		config_.emission.maxParticles = std::clamp(
			config_.emission.maxParticles,
			1u,
			kMaximumParticleCountPerEmitter
		);
		random_.SetSeed(randomSeed);
		simulator_.Initialize(config_.emission.maxParticles);
		trails_.clear();
		return true;
	}

	void CpuParticleEmitterRuntime::Update(float deltaTime, const Transform3D& emitterTransform) {
		(void)emitterTransform;
		simulator_.Update(deltaTime, config_);
		UpdateTrails(deltaTime);
	}

	void CpuParticleEmitterRuntime::Emit(uint32_t count, const Transform3D& emitterTransform) {
		simulator_.Emit(config_, emitterTransform, count, random_);
		UpdateTrails(0.0f);
	}

	void CpuParticleEmitterRuntime::SetTransform(const Transform3D& emitterTransform) {
		(void)emitterTransform;
	}

	void CpuParticleEmitterRuntime::Stop(StopMode mode) {

		// Immediateだけ生存Particleを破棄し、Finishでは寿命完了まで更新を継続
		if (mode == StopMode::Immediate) {
			simulator_.Reset();
			trails_.clear();
		}
	}

	void CpuParticleEmitterRuntime::Reset() {
		simulator_.Reset();
		trails_.clear();
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

	void CpuParticleEmitterRuntime::SubmitTrailRenderData(
		MadoEngine::Ribbon::RibbonEffectRenderer3d& renderer,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) const {
		if (!config_.trail.isEnabled || trails_.empty()) {
			return;
		}

		const Matrix4x4 emitterMatrix = Matrix::MakeAffine(
			emitterTransform.scale,
			emitterTransform.rotate,
			emitterTransform.translate
		);
		const MadoEngine::Effect::EffectTrack<float> widthTrack = CreateTrailWidthTrack(
			config_.trail.startWidth,
			config_.trail.endWidth
		);
		const MadoEngine::Effect::EffectTrack<Vector4> colorTrack = CreateTrailColorTrack(
			config_.trail.startColor,
			config_.trail.endColor
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
				appendLatestPosition && state.points.size() >= config_.trail.maxPointCount
				? 1
				: 0;

			MadoEngine::Ribbon::RibbonRenderData renderData;
			renderData.points.reserve(
				state.points.size() - firstPointIndex + (appendLatestPosition ? 1u : 0u)
			);
			for (std::size_t pointIndex = firstPointIndex; pointIndex < state.points.size(); ++pointIndex) {
				const TrailPoint& point = state.points[pointIndex];
				renderData.points.push_back({
					config_.simulationSpace == SimulationSpace::Local
						? Matrix::Transform(point.position, emitterMatrix)
						: point.position,
					point.age,
					config_.trail.pointLifetime,
				});
			}
			if (appendLatestPosition) {

				// 最小間隔未満でも描画時の先端だけは現在位置へ追従させて軌跡の遅延を抑制
				renderData.points.push_back({
					config_.simulationSpace == SimulationSpace::Local
						? Matrix::Transform(state.latestPosition, emitterMatrix)
						: state.latestPosition,
					0.0f,
					config_.trail.pointLifetime,
				});
			}
			if (renderData.points.size() < kMinimumParticleTrailPointCount) {
				continue;
			}

			renderData.widthOverLifetime = widthTrack;
			renderData.colorOverLifetime = colorTrack;
			renderData.interpolation = config_.trail.interpolation == ParticleTrailInterpolation::CatmullRom
				? MadoEngine::Ribbon::RibbonInterpolationMode::CatmullRom
				: MadoEngine::Ribbon::RibbonInterpolationMode::Linear;
			renderData.smoothingSubdivision = config_.trail.smoothingSubdivision;
			renderData.cameraFacing = config_.trail.cameraFacing;
			renderData.textureName = config_.trail.textureName;
			renderData.blendMode = config_.trail.blendMode;
			renderData.cullMode = MadoEngine::Render::CullMode::None;
			renderData.renderLayer = renderLayer;
			renderer.Submit(renderData);
		}
	}

	bool CpuParticleEmitterRuntime::IsIdle() const {
		return simulator_.GetAliveCount() == 0 && trails_.empty();
	}

	void CpuParticleEmitterRuntime::UpdateTrails(float deltaTime) {
		if (!config_.trail.isEnabled) {
			trails_.clear();
			return;
		}

		const float safeDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			0.1f
		);
		for (auto& [identifier, state] : trails_) {
			(void)identifier;
			state.wasParticleAlive = state.isParticleAlive;
			state.isParticleAlive = false;
			for (TrailPoint& point : state.points) {
				point.age += safeDeltaTime;
			}
			std::erase_if(state.points, [this](const TrailPoint& point) {
				return point.age >= config_.trail.pointLifetime;
			});
		}

		// Simulatorの交換削除に依存せず安定IDから粒子固有の履歴へ位置を反映
		for (const ParticleState& particle : simulator_.GetParticles()) {
			TrailState& state = trails_[particle.identifier];
			state.isParticleAlive = true;
			state.latestPosition = particle.position;
			state.hasLatestPosition = true;
			TryAddTrailPoint(state, particle.position);
		}

		for (auto& [identifier, state] : trails_) {
			(void)identifier;
			if (state.wasParticleAlive && !state.isParticleAlive && state.hasLatestPosition) {

				// 粒子消滅時の最終位置を固定し、残存Trailだけを寿命まで減衰
				TryAddTrailPoint(state, state.latestPosition);
			}
		}
		std::erase_if(trails_, [](const auto& entry) {
			const TrailState& state = entry.second;
			return !state.isParticleAlive && state.points.empty();
		});
	}

	void CpuParticleEmitterRuntime::TryAddTrailPoint(
		TrailState& state,
		const Vector3& position) {
		if (!IsFiniteVector3(position)) {
			return;
		}

		const float minimumDistance = (std::max)(
			config_.trail.minPointDistance,
			kParticleTrailPositionEpsilon
		);
		if (!state.points.empty()) {
			const Vector3 difference = position - state.points.back().position;
			if (difference.LengthSq() < minimumDistance * minimumDistance) {

				// 微小移動によるPoint過密化とRibbon Meshの不要な増加を抑制
				return;
			}
		}

		state.points.push_back({ position, 0.0f });
		while (state.points.size() > config_.trail.maxPointCount) {
			state.points.erase(state.points.begin());
		}
	}

} // MadoEngine::Particle名前空間
