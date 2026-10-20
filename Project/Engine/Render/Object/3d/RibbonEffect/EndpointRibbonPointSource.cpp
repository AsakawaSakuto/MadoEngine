#include "EndpointRibbonPointSource.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>

namespace {

	/// @brief 全成分が有限な座標か確認
	/// @param position 確認対象座標
	/// @return 全成分が有限な場合はtrue
	bool IsFinitePosition(const Vector3& position) {
		return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
	}

}

namespace MadoEngine::Ribbon {

	EndpointRibbonPointSource::EndpointRibbonPointSource(
		const RibbonEndpointModule& config, const Transform3D& transform) : config_(config) {
		SetTransform(transform);
	}

	void EndpointRibbonPointSource::Update(float deltaTime) {
		(void)deltaTime;
	}

	void EndpointRibbonPointSource::SetTransform(const Transform3D& transform) {

		// 明示指定したWorld始終点は後続の基準Transform更新から独立
		if (!isGenerating_ || hasWorldOverride_) {
			return;
		}
		const Matrix4x4 world = Matrix::MakeAffine(transform.scale, transform.rotate, transform.translate);
		worldEndpoints_.startPosition = Matrix::Transform(config_.positions.startPosition, world);
		worldEndpoints_.endPosition = Matrix::Transform(config_.positions.endPosition, world);
		RebuildPoints();
	}

	bool EndpointRibbonPointSource::SetEndpoints(const RibbonEndpoints& endpoints) {

		// 不正な座標で直前の有効な形状を破壊しないため入力全体を先に検証
		if (!isGenerating_ || !IsFinitePosition(endpoints.startPosition) || !IsFinitePosition(endpoints.endPosition)) {
			return false;
		}
		hasWorldOverride_ = true;
		worldEndpoints_ = endpoints;
		RebuildPoints();
		return true;
	}

	void EndpointRibbonPointSource::Stop(RibbonStopMode mode) {
		(void)mode;

		// 履歴を持たない固定形状は再生期間の終端で終了し旧2点間表示の寿命を維持
		isGenerating_ = false;
		Clear();
	}

	void EndpointRibbonPointSource::Clear() {
		points_.clear();
	}

	void EndpointRibbonPointSource::RebuildPoints() {
		points_.clear();
		if (!IsFinitePosition(worldEndpoints_.startPosition) || !IsFinitePosition(worldEndpoints_.endPosition)) {
			return;
		}

		// 時間と長さ方向を分離し、固定形状の点の経過時間は常に0として保持
		const uint32_t segments = std::clamp(config_.segmentCount, 1u, kMaximumRibbonPointCount - 1);
		points_.reserve(static_cast<std::size_t>(segments) + 1);
		for (uint32_t index = 0; index <= segments; ++index) {
			const float rate = static_cast<float>(index) / static_cast<float>(segments);
			RibbonPoint point;
			point.position = worldEndpoints_.startPosition * (1.0f - rate) + worldEndpoints_.endPosition * rate;
			points_.push_back(point);
		}
	}

} // namespace MadoEngine::Ribbon
