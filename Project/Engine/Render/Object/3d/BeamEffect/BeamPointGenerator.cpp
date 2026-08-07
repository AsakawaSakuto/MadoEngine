#include "BeamPointGenerator.h"
#include <algorithm>
#include <cmath>

namespace {

	constexpr float kDirectionEpsilon = 1.0e-6f;

	/// @brief Vector3の全成分が有限値か確認する
	/// @param value 確認対象
	/// @return 全成分が有限値の場合はtrue
	bool IsFinite(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	/// @brief 2つのVector3の外積を計算する
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

	/// @brief Vector3を安全に正規化する
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

	/// @brief 線形補間を行う
	/// @param lhs 始点
	/// @param rhs 終点
	/// @param rate 補間率
	/// @return 補間結果
	Vector3 Lerp(const Vector3& lhs, const Vector3& rhs, float rate) {
		return lhs * (1.0f - rate) + rhs * rate;
	}

} // namespace

namespace MadoEngine::Beam {

	std::vector<MadoEngine::Ribbon::RibbonPoint> BeamPointGenerator::Generate(
		const Vector3& startPosition,
		const Vector3& endPosition,
		const BeamGeometryModule& geometry,
		const BeamNoiseModule& noise,
		float elapsedTime) const {
		if (!IsFinite(startPosition) || !IsFinite(endPosition)) {
			return {};
		}

		const uint32_t segmentCount = std::clamp(
			geometry.segmentCount,
			kMinimumBeamSegmentCount,
			kMaximumBeamSegmentCount
		);
		const Vector3 beamVector = endPosition - startPosition;
		const Vector3 direction = SafeNormalize(beamVector, { 0.0f, 1.0f, 0.0f });
		const Vector3 referenceAxis = std::abs(direction.y) < 0.9f
			? Vector3{ 0.0f, 1.0f, 0.0f }
			: Vector3{ 1.0f, 0.0f, 0.0f };
		const Vector3 noiseAxisX = SafeNormalize(Cross(direction, referenceAxis), { 1.0f, 0.0f, 0.0f });
		const Vector3 noiseAxisY = SafeNormalize(Cross(direction, noiseAxisX), { 0.0f, 0.0f, 1.0f });

		const float safeTime = std::isfinite(elapsedTime) ? elapsedTime : 0.0f;
		const float noiseCoordinateOffset = safeTime * noise.scrollSpeed;
		std::vector<MadoEngine::Ribbon::RibbonPoint> points;
		points.reserve(static_cast<std::size_t>(segmentCount) + 1);
		for (uint32_t index = 0; index <= segmentCount; ++index) {
			const float rate = static_cast<float>(index) / static_cast<float>(segmentCount);
			Vector3 position = Lerp(startPosition, endPosition, rate);
			if (index != 0 && index != segmentCount && noise.amplitude > 0.0f) {
				const float envelope = std::sin(rate * 3.14159265358979323846f);
				const float coordinate = rate * noise.frequency + noiseCoordinateOffset;
				const float offsetX = EvaluateNoise(coordinate, noise.seed);
				const float offsetY = EvaluateNoise(coordinate + 31.416f, noise.seed ^ 0x9e3779b9u);
				position += (noiseAxisX * offsetX + noiseAxisY * offsetY) * (noise.amplitude * envelope);
			}
			if (!IsFinite(position)) {
				position = Lerp(startPosition, endPosition, rate);
			}
			points.push_back({ position, rate, 1.0f });
		}
		return points;
	}

	float BeamPointGenerator::EvaluateNoise(float coordinate, uint32_t seed) const {
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

} // namespace MadoEngine::Beam
