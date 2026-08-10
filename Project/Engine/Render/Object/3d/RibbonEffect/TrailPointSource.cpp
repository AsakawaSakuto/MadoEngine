#include "TrailPointSource.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>

namespace {

	constexpr float kRibbonPointDistanceEpsilon = 0.000001f;

	/// @brief Vector3の全要素が有限値か確認
	/// @param value 確認対象
	/// @return 全要素が有限値の場合はtrue
	bool IsFiniteVector3(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

} // namespace

namespace MadoEngine::Ribbon {

	TrailPointSource::TrailPointSource(
		const RibbonTrailModule& config,
		const Transform3D& initialTransform)
		: config_(config) {
		localToWorld_ = Matrix::MakeAffine(
			initialTransform.scale,
			initialTransform.rotate,
			initialTransform.translate
		);
		worldToLocal_ = Matrix::Inverse(localToWorld_);
		TryAddPoint(initialTransform.translate);
	}

	void TrailPointSource::Update(float deltaTime) {
		const float safeDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			0.1f
		);
		for (RibbonPoint& point : sourcePoints_) {
			point.age += safeDeltaTime;
		}

		sourcePoints_.erase(
			std::remove_if(
				sourcePoints_.begin(),
				sourcePoints_.end(),
				[](const RibbonPoint& point) {
					return point.age >= point.lifetime;
				}
			),
			sourcePoints_.end()
		);
		RebuildWorldPoints();
	}

	void TrailPointSource::SetTransform(const Transform3D& transform) {
		if (!isGenerating_) {
			return;
		}
		TryAddPoint(transform.translate);
	}

	void TrailPointSource::Stop(RibbonStopMode mode) {
		isGenerating_ = false;
		if (mode == RibbonStopMode::Immediate) {
			Clear();
		}
	}

	void TrailPointSource::Clear() {
		sourcePoints_.clear();
		worldPoints_.clear();
	}

	void TrailPointSource::TryAddPoint(const Vector3& worldPosition) {
		if (!IsFiniteVector3(worldPosition)) {
			return;
		}

		const float minimumDistance = (std::max)(config_.minPointDistance, kRibbonPointDistanceEpsilon);
		if (!worldPoints_.empty()) {
			const Vector3 difference = worldPosition - worldPoints_.back().position;
			if (difference.LengthSq() < minimumDistance * minimumDistance) {
				return;
			}
		}

		RibbonPoint point;
		point.position = config_.simulationSpace == RibbonSimulationSpace::Local
			? Matrix::Transform(worldPosition, worldToLocal_)
			: worldPosition;
		point.age = 0.0f;
		point.lifetime = config_.pointLifetime;
		sourcePoints_.push_back(point);

		while (sourcePoints_.size() > config_.maxPointCount) {
			sourcePoints_.erase(sourcePoints_.begin());
		}
		RebuildWorldPoints();
	}

	void TrailPointSource::RebuildWorldPoints() {
		worldPoints_ = sourcePoints_;
		if (config_.simulationSpace != RibbonSimulationSpace::Local) {
			return;
		}

		for (RibbonPoint& point : worldPoints_) {
			point.position = Matrix::Transform(point.position, localToWorld_);
		}
	}

} // namespace MadoEngine::Ribbon
