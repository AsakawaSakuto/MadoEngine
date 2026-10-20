#include "RibbonPointModifier.h"
#include <algorithm>
#include <cmath>

namespace {

	constexpr float kDirectionEpsilon = 1.0e-6f;

	/// @brief Vector3の全成分が有限値か確認
	/// @param value 確認対象
	/// @return 全成分が有限値の場合はtrue
	bool IsFinite(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	/// @brief 2つのVector3の外積を計算
	/// @param lhs 左辺
	/// @param rhs 右辺
	/// @return 外積
	Vector3 Cross(const Vector3& lhs, const Vector3& rhs) {
		return {
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x,
		};
	}

	/// @brief Vector3を安全に正規化
	/// @param value 正規化対象
	/// @param fallback 正規化できない場合の値
	/// @return 正規化後Vector3
	Vector3 SafeNormalize(const Vector3& value, const Vector3& fallback) {
		const float lengthSquared = value.LengthSq();
		if (!std::isfinite(lengthSquared) || lengthSquared <= kDirectionEpsilon * kDirectionEpsilon) {
			return fallback;
		}
		return value / std::sqrt(lengthSquared);
	}

} // namespace

namespace MadoEngine::Ribbon {

	void RibbonPointModifier::ApplyNoise(
		std::vector<RibbonPoint>& points, const RibbonNoiseModule& noise, float elapsedTime) const {
		if (points.size() < 3 || !std::isfinite(noise.amplitude) || noise.amplitude <= 0.0f) {
			return;
		}

		// 描画用Copyだけを変形し、履歴生成の距離判定と点の寿命からNoiseを分離
		const std::vector<RibbonPoint> original = points;
		std::vector<float> distances(points.size(), 0.0f);
		for (std::size_t index = 1; index < points.size(); ++index) {
			distances[index] = distances[index - 1] + (original[index].position - original[index - 1].position).Length();
		}
		const float totalLength = distances.back();
		if (!std::isfinite(totalLength) || totalLength <= kDirectionEpsilon) {
			return;
		}
		float time = 0.0f;
		if (std::isfinite(elapsedTime)) {
			time = elapsedTime;
		}

		// 曲がる軌跡でも各点の接線に直交する面へ変位し、両端は固定
		for (std::size_t index = 1; index + 1 < points.size(); ++index) {
			const float rate = distances[index] / totalLength;
			const Vector3 direction = SafeNormalize(original[index + 1].position - original[index - 1].position, { 0.0f, 1.0f, 0.0f });
			Vector3 reference{ 1.0f, 0.0f, 0.0f };
			if (std::abs(direction.y) < 0.9f) {
				reference = { 0.0f, 1.0f, 0.0f };
			}
			const Vector3 axisX = SafeNormalize(Cross(direction, reference), { 1.0f, 0.0f, 0.0f });
			const Vector3 axisY = SafeNormalize(Cross(direction, axisX), { 0.0f, 0.0f, 1.0f });
			const float coordinate = rate * noise.frequency + time * noise.scrollSpeed;
			const float offsetX = EvaluateNoise(coordinate, noise.seed);
			const float offsetY = EvaluateNoise(coordinate + 31.416f, noise.seed ^ 0x9e3779b9u);
			const float envelope = std::sin(rate * 3.14159265358979323846f);
			const Vector3 position = original[index].position + (axisX * offsetX + axisY * offsetY) * (noise.amplitude * envelope);
			if (IsFinite(position)) {
				points[index].position = position;
			}
		}
	}

	float RibbonPointModifier::EvaluateNoise(float coordinate, uint32_t seed) const {
		if (!std::isfinite(coordinate)) {
			return 0.0f;
		}
		const float floorCoordinate = std::floor(coordinate);
		const int32_t leftIndex = static_cast<int32_t>(std::clamp(
			static_cast<double>(floorCoordinate),
			static_cast<double>((std::numeric_limits<int32_t>::min)() + 1),
			static_cast<double>((std::numeric_limits<int32_t>::max)() - 1)
		));
		const float fraction = std::clamp(coordinate - floorCoordinate, 0.0f, 1.0f);

		// 格子境界で傾きが不連続にならないSmoothstepによるHash値補間
		const float smoothFraction = fraction * fraction * (3.0f - 2.0f * fraction);
		const auto hashToSigned = [seed](int32_t index) {
			uint32_t value = static_cast<uint32_t>(index) ^ seed;
			value ^= value >> 16;
			value *= 0x7feb352du;
			value ^= value >> 15;
			value *= 0x846ca68bu;
			value ^= value >> 16;
			return static_cast<float>(value & 0x00ffffffu) / 8388607.5f - 1.0f;
		};
		const float left = hashToSigned(leftIndex);
		const float right = hashToSigned(leftIndex + 1);
		return left * (1.0f - smoothFraction) + right * smoothFraction;
	}

} // namespace MadoEngine::Ribbon
