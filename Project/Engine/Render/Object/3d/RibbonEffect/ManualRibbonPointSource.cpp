#include "ManualRibbonPointSource.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>

namespace {

	/// @brief Vector3の全要素が有限値か確認
	/// @param value 確認対象
	/// @return 全要素が有限値の場合はtrue
	bool IsFiniteVector3(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

} // namespace

namespace MadoEngine::Ribbon {

	ManualRibbonPointSource::ManualRibbonPointSource(
		const RibbonTrailModule& config,
		const Transform3D& initialTransform)
		: config_(config), transform_(initialTransform) {
		SetControlPoints(config_.defaultControlPoints);
	}

	void ManualRibbonPointSource::Update(float deltaTime) {
		if (isGenerating_) {

			// 再生中は手動制御点を固定形状として維持するため寿命を進めず更新
			for (RibbonPoint& point : sourcePoints_) {
				point.age = 0.0f;
			}
			RebuildWorldPoints();
			return;
		}

		// Finish停止後は既存形状だけを寿命に従って自然消滅
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

	void ManualRibbonPointSource::SetTransform(const Transform3D& transform) {
		transform_ = transform;
		RebuildWorldPoints();
	}

	void ManualRibbonPointSource::Stop(RibbonStopMode mode) {
		isGenerating_ = false;
		if (mode == RibbonStopMode::Immediate) {
			Clear();
		}
	}

	void ManualRibbonPointSource::Clear() {
		sourcePoints_.clear();
		worldPoints_.clear();
	}

	bool ManualRibbonPointSource::SetControlPoints(const std::vector<Vector3>& controlPoints) {
		if (!isGenerating_) {
			return false;
		}

		sourcePoints_.clear();
		const std::size_t pointCount = (std::min)(
			controlPoints.size(),
			static_cast<std::size_t>(config_.maxPointCount)
		);
		sourcePoints_.reserve(pointCount);
		for (std::size_t index = 0; index < pointCount; ++index) {
			if (!IsFiniteVector3(controlPoints[index])) {
				continue;
			}
			RibbonPoint point;
			point.position = controlPoints[index];
			point.age = 0.0f;
			point.lifetime = config_.pointLifetime;
			sourcePoints_.push_back(point);
		}
		RebuildWorldPoints();
		return !sourcePoints_.empty();
	}

	void ManualRibbonPointSource::RebuildWorldPoints() {

		// 入力されたLocal制御点を保持したまま描画用CopyだけをWorld座標へ変換
		worldPoints_ = sourcePoints_;
		if (config_.simulationSpace != RibbonSimulationSpace::Local) {
			return;
		}

		const Matrix4x4 world = Matrix::MakeAffine(
			transform_.scale,
			transform_.rotate,
			transform_.translate
		);
		for (RibbonPoint& point : worldPoints_) {
			point.position = Matrix::Transform(point.position, world);
		}
	}

} // namespace MadoEngine::Ribbon
