#include "CpuParticleSimulator.h"
#include "Math/Function/MathFunction.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <type_traits>

namespace {

	using namespace MadoEngine::Particle;

	struct ShapeSample {
		Vector3 position{};
		Vector3 outward{};
	};

	/// @brief float範囲から乱数を生成する
	/// @param range 乱数範囲
	/// @param random 乱数生成器
	/// @return 生成した値
	float SampleRange(const ValueRange<float>& range, Random& random) {
		return random.Float(range.min, range.max);
	}

	/// @brief Vector2範囲から乱数を生成する
	/// @param range 乱数範囲
	/// @param random 乱数生成器
	/// @return 生成した値
	Vector2 SampleRange(const ValueRange<Vector2>& range, Random& random) {
		return {
			random.Float(range.min.x, range.max.x),
			random.Float(range.min.y, range.max.y),
		};
	}

	/// @brief Vector3範囲から乱数を生成する
	/// @param range 乱数範囲
	/// @param random 乱数生成器
	/// @return 生成した値
	Vector3 SampleRange(const ValueRange<Vector3>& range, Random& random) {
		return {
			random.Float(range.min.x, range.max.x),
			random.Float(range.min.y, range.max.y),
			random.Float(range.min.z, range.max.z),
		};
	}

	/// @brief Vector4範囲から乱数を生成する
	/// @param range 乱数範囲
	/// @param random 乱数生成器
	/// @return 生成した値
	Vector4 SampleRange(const ValueRange<Vector4>& range, Random& random) {
		return {
			random.Float(range.min.x, range.max.x),
			random.Float(range.min.y, range.max.y),
			random.Float(range.min.z, range.max.z),
			random.Float(range.min.w, range.max.w),
		};
	}

	/// @brief 球面上の一様な方向を生成する
	/// @param random 乱数生成器
	/// @return 生成した単位方向
	Vector3 SampleUnitSphere(Random& random) {
		const float z = random.Float(-1.0f, 1.0f);
		const float angle = random.Float(0.0f, 2.0f * std::numbers::pi_v<float>);
		const float radius = std::sqrt((std::max)(0.0f, 1.0f - z * z));
		return { radius * std::cos(angle), z, radius * std::sin(angle) };
	}

	/// @brief 平面またはRing用の直交基底を生成する
	/// @param normal 平面法線
	/// @param outTangent 接線の出力先
	/// @param outBitangent 従法線の出力先
	void BuildPlaneBasis(const Vector3& normal, Vector3& outTangent, Vector3& outBitangent) {
		const Vector3 normalized = Math::Normalize(normal);
		const Vector3 reference = std::abs(normalized.y) < 0.999f
			? Vector3{ 0.0f, 1.0f, 0.0f }
			: Vector3{ 1.0f, 0.0f, 0.0f };
		outTangent = Math::Normalize(Math::Cross(reference, normalized));
		outBitangent = Math::Normalize(Math::Cross(normalized, outTangent));
	}

	/// @brief 発生形状から座標と外向き方向を生成する
	/// @param shape 発生形状
	/// @param random 乱数生成器
	/// @return 生成した形状サンプル
	ShapeSample SampleShape(const ParticleShape& shape, Random& random) {
		return std::visit([&random](const auto& value) -> ShapeSample {
			using ShapeType = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<ShapeType, PointShape>) {
				return { value.offset, {} };
			} else if constexpr (std::is_same_v<ShapeType, LineShape>) {
				const float t = random.Float(0.0f, 1.0f);
				return { value.start + (value.end - value.start) * t, {} };
			} else if constexpr (std::is_same_v<ShapeType, SphereShape>) {
				const Vector3 direction = SampleUnitSphere(random);
				const float radiusScale = value.emitFromSurface ? 1.0f : std::cbrt(random.Float(0.0f, 1.0f));
				return { direction * (value.radius * radiusScale), direction };
			} else if constexpr (std::is_same_v<ShapeType, BoxShape>) {
				ShapeSample sample;
				sample.position = {
					random.Float(-value.halfExtents.x, value.halfExtents.x),
					random.Float(-value.halfExtents.y, value.halfExtents.y),
					random.Float(-value.halfExtents.z, value.halfExtents.z),
				};
				if (value.emitFromSurface) {
					const int axis = random.Int(0, 2);
					const float sign = random.Int(0, 1) == 0 ? -1.0f : 1.0f;
					sample.position[static_cast<std::size_t>(axis)] = value.halfExtents[static_cast<std::size_t>(axis)] * sign;
					sample.outward[static_cast<std::size_t>(axis)] = sign;
				} else {
					sample.outward = Math::Normalize(sample.position);
				}
				return sample;
			} else if constexpr (std::is_same_v<ShapeType, PlaneShape>) {
				Vector3 tangent;
				Vector3 bitangent;
				BuildPlaneBasis(value.normal, tangent, bitangent);
				const float x = random.Float(-value.halfExtents.x, value.halfExtents.x);
				const float y = random.Float(-value.halfExtents.y, value.halfExtents.y);
				return { tangent * x + bitangent * y, Math::Normalize(value.normal) };
			} else {
				Vector3 tangent;
				Vector3 bitangent;
				BuildPlaneBasis(value.normal, tangent, bitangent);
				const float angle = random.Float(0.0f, 2.0f * std::numbers::pi_v<float>);
				const float radius = value.emitFromEdge
					? value.outerRadius
					: std::sqrt(random.Float(value.innerRadius * value.innerRadius, value.outerRadius * value.outerRadius));
				const Vector3 radial = tangent * std::cos(angle) + bitangent * std::sin(angle);
				return { radial * radius, radial };
			}
		}, shape);
	}

	/// @brief ゼロベクトルの場合に既定方向を返す
	/// @param direction 判定する方向
	/// @return 正規化した方向
	Vector3 NormalizeDirection(const Vector3& direction) {
		if (direction.LengthSq() <= 0.000001f) {
			return { 0.0f, 1.0f, 0.0f };
		}

		return Math::Normalize(direction);
	}

} // namespace

namespace MadoEngine::Particle {

	void CpuParticleSimulator::Initialize(uint32_t maxParticles) {
		particles_.clear();
		particles_.resize((std::max)(1u, maxParticles));
		aliveCount_ = 0;
	}

	void CpuParticleSimulator::Reset() {
		aliveCount_ = 0;
	}

	void CpuParticleSimulator::Emit(
		const EmitterConfig& config,
		const Transform3D& emitterTransform,
		uint32_t count,
		Random& random) {
		if (particles_.empty() || count == 0) {
			return;
		}

		const uint32_t spawnCount = (std::min)(count, static_cast<uint32_t>(particles_.size()) - aliveCount_);
		const Matrix4x4 emitterMatrix = Matrix::MakeAffine(
			emitterTransform.scale,
			emitterTransform.rotate,
			emitterTransform.translate
		);
		const Matrix4x4 emitterRotation = Matrix::MakeAffine(
			{ 1.0f, 1.0f, 1.0f },
			emitterTransform.rotate,
			{}
		);

		for (uint32_t index = 0; index < spawnCount; ++index) {
			const ShapeSample shapeSample = SampleShape(config.shape, random);
			const Vector3 configuredDirection = NormalizeDirection(SampleRange(config.initial.direction, random));
			Vector3 direction = config.initial.directionMode == DirectionMode::ShapeOutward && shapeSample.outward.LengthSq() > 0.000001f
				? NormalizeDirection(shapeSample.outward)
				: configuredDirection;

			ParticleState& particle = particles_[aliveCount_++];
			particle = {};
			particle.position = shapeSample.position;
			particle.velocity = direction * SampleRange(config.initial.speed, random);
			particle.lifeTime = SampleRange(config.initial.lifeTime, random);
			particle.rotation = SampleRange(config.initial.rotation, random);
			particle.angularVelocity = SampleRange(config.initial.angularVelocity, random);
			particle.startScale = SampleRange(config.sizeOverLifetime.start, random);
			particle.endScale = SampleRange(config.sizeOverLifetime.end, random);
			particle.scale = particle.startScale;
			particle.startColor = SampleRange(config.colorOverLifetime.start, random);
			particle.endColor = SampleRange(config.colorOverLifetime.end, random);
			particle.color = particle.startColor;

			if (config.simulationSpace == SimulationSpace::World) {
				particle.position = Matrix::Transform(particle.position, emitterMatrix);
				particle.velocity = Matrix::Transform(particle.velocity, emitterRotation);
				particle.rotation += emitterTransform.rotate.z;
				particle.startScale.x *= std::abs(emitterTransform.scale.x);
				particle.startScale.y *= std::abs(emitterTransform.scale.y);
				particle.endScale.x *= std::abs(emitterTransform.scale.x);
				particle.endScale.y *= std::abs(emitterTransform.scale.y);
				particle.scale = particle.startScale;
			}
		}
	}

	void CpuParticleSimulator::Update(float deltaTime, const EmitterConfig& config) {
		if (deltaTime <= 0.0f || aliveCount_ == 0) {
			return;
		}

		const Vector3 totalAcceleration = config.motion.gravity + config.motion.acceleration;
		const float dragFactor = std::exp(-config.motion.drag * deltaTime);

		uint32_t index = 0;
		while (index < aliveCount_) {
			ParticleState& particle = particles_[index];
			particle.age += deltaTime;
			if (particle.age >= particle.lifeTime) {
				particles_[index] = particles_[aliveCount_ - 1];
				--aliveCount_;
				continue;
			}

			particle.velocity += totalAcceleration * deltaTime;
			particle.velocity *= dragFactor;
			particle.position += particle.velocity * deltaTime;
			particle.rotation += particle.angularVelocity * deltaTime;

			const float normalizedAge = std::clamp(particle.age / particle.lifeTime, 0.0f, 1.0f);
			particle.scale = particle.startScale + (particle.endScale - particle.startScale) * normalizedAge;
			particle.color = particle.startColor + (particle.endColor - particle.startColor) * normalizedAge;
			++index;
		}
	}

	std::span<const ParticleState> CpuParticleSimulator::GetParticles() const {
		return std::span<const ParticleState>(particles_.data(), aliveCount_);
	}

} // namespace MadoEngine::Particle
