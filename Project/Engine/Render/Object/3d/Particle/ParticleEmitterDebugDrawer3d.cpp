#include "ParticleEmitterDebugDrawer3d.h"
#include "Math/Function/MathFunction.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/3d/Line/MyDebugLine.h"
#include <cmath>
#include <numbers>
#include <type_traits>

namespace {

	using namespace MadoEngine::Particle;

	constexpr int kCircleSegmentCount = 32;
	constexpr float kPointMarkerSize = 0.1f;
	constexpr float kNormalLineLength = 0.5f;
	constexpr float kMinimumLengthSquared = 0.000001f;
	constexpr Vector4 kNormalColor = { 1.0f, 1.0f, 0.0f, 1.0f };

	/// @brief ゼロベクトルを考慮して方向を正規化する
	/// @param direction 正規化する方向
	/// @return 正規化した方向
	Vector3 NormalizeDirection(const Vector3& direction) {
		if (direction.LengthSq() <= kMinimumLengthSquared) {
			return { 0.0f, 1.0f, 0.0f };
		}

		return Math::Normalize(direction);
	}

	/// @brief 平面上の直交基底を生成する
	/// @param normal 平面法線
	/// @param outTangent 接線の出力先
	/// @param outBitangent 従法線の出力先
	void BuildPlaneBasis(const Vector3& normal, Vector3& outTangent, Vector3& outBitangent) {
		const Vector3 normalized = NormalizeDirection(normal);
		const Vector3 reference = std::abs(normalized.y) < 0.999f
			? Vector3{ 0.0f, 1.0f, 0.0f }
			: Vector3{ 1.0f, 0.0f, 0.0f };
		outTangent = NormalizeDirection(Math::Cross(reference, normalized));
		outBitangent = NormalizeDirection(Math::Cross(normalized, outTangent));
	}

	/// @brief 2点間の線分をDebugLineへ登録する
	/// @param start 線分の開始位置
	/// @param end 線分の終了位置
	/// @param color 表示色
	void SubmitSegment(const Vector3& start, const Vector3& end, const Vector4& color) {
		Segment segment;
		segment.origin = start;
		segment.diff = end - start;
		MyDebugLine::AddShape(segment, color);
	}

	/// @brief Transformを適用した円をDebugLineへ登録する
	/// @param normal 円のローカル法線
	/// @param radius 円のローカル半径
	/// @param worldMatrix EmitterのWorld行列
	/// @param color 表示色
	void SubmitCircle(
		const Vector3& normal,
		float radius,
		const Matrix4x4& worldMatrix,
		const Vector4& color) {
		if (radius <= 0.0f) {
			return;
		}

		Vector3 tangent;
		Vector3 bitangent;
		BuildPlaneBasis(normal, tangent, bitangent);
		for (int index = 0; index < kCircleSegmentCount; ++index) {
			const float angle0 =
				2.0f * std::numbers::pi_v<float> * static_cast<float>(index) /
				static_cast<float>(kCircleSegmentCount);
			const float angle1 =
				2.0f * std::numbers::pi_v<float> * static_cast<float>(index + 1) /
				static_cast<float>(kCircleSegmentCount);
			const Vector3 localStart =
				(tangent * std::cos(angle0) + bitangent * std::sin(angle0)) * radius;
			const Vector3 localEnd =
				(tangent * std::cos(angle1) + bitangent * std::sin(angle1)) * radius;
			SubmitSegment(
				Matrix::Transform(localStart, worldMatrix),
				Matrix::Transform(localEnd, worldMatrix),
				color
			);
		}
	}

	/// @brief 平面形状をDebugLineへ登録する
	/// @param shape 表示する平面形状
	/// @param worldMatrix EmitterのWorld行列
	/// @param rotationMatrix Emitterの回転行列
	/// @param color 表示色
	void SubmitPlane(
		const PlaneShape& shape,
		const Matrix4x4& worldMatrix,
		const Matrix4x4& rotationMatrix,
		const Vector4& color) {
		Vector3 tangent;
		Vector3 bitangent;
		BuildPlaneBasis(shape.normal, tangent, bitangent);
		const Vector3 localCorners[4] = {
			tangent * -shape.halfExtents.x + bitangent * -shape.halfExtents.y,
			tangent *  shape.halfExtents.x + bitangent * -shape.halfExtents.y,
			tangent *  shape.halfExtents.x + bitangent *  shape.halfExtents.y,
			tangent * -shape.halfExtents.x + bitangent *  shape.halfExtents.y,
		};
		Vector3 worldCorners[4];
		for (int index = 0; index < 4; ++index) {
			worldCorners[index] = Matrix::Transform(localCorners[index], worldMatrix);
		}
		for (int index = 0; index < 4; ++index) {
			SubmitSegment(worldCorners[index], worldCorners[(index + 1) % 4], color);
		}

		const Vector3 center = Matrix::Transform({}, worldMatrix);
		const Vector3 worldNormal = NormalizeDirection(Matrix::Transform(shape.normal, rotationMatrix));
		SubmitSegment(center, center + worldNormal * kNormalLineLength, kNormalColor);
	}

} // namespace

namespace MadoEngine::Particle {

	void ParticleEmitterDebugDrawer3d::Submit(
		const EmitterConfig& config,
		const Transform3D& transform,
		const Vector4& color) {
		const Matrix4x4 worldMatrix = Matrix::MakeAffine(
			transform.scale,
			transform.rotate,
			transform.translate
		);
		const Matrix4x4 rotationMatrix = Matrix::MakeAffine(
			{ 1.0f, 1.0f, 1.0f },
			transform.rotate,
			{}
		);

		std::visit([&](const auto& shape) {
			using ShapeType = std::decay_t<decltype(shape)>;
			if constexpr (std::is_same_v<ShapeType, PointShape>) {
				const Vector3 axes[3] = {
					{ kPointMarkerSize, 0.0f, 0.0f },
					{ 0.0f, kPointMarkerSize, 0.0f },
					{ 0.0f, 0.0f, kPointMarkerSize },
				};
				for (const Vector3& axis : axes) {
					SubmitSegment(
						Matrix::Transform(shape.offset - axis, worldMatrix),
						Matrix::Transform(shape.offset + axis, worldMatrix),
						color
					);
				}
			} else if constexpr (std::is_same_v<ShapeType, LineShape>) {
				SubmitSegment(
					Matrix::Transform(shape.start, worldMatrix),
					Matrix::Transform(shape.end, worldMatrix),
					color
				);
			} else if constexpr (std::is_same_v<ShapeType, SphereShape>) {
				OvalSphere sphere;
				sphere.center = Matrix::Transform({}, worldMatrix);
				sphere.radius = {
					shape.radius * std::abs(transform.scale.x),
					shape.radius * std::abs(transform.scale.y),
					shape.radius * std::abs(transform.scale.z),
				};
				sphere.rotate = transform.rotate;
				sphere.UpdateOrientation();
				MyDebugLine::AddShape(sphere, color);
			} else if constexpr (std::is_same_v<ShapeType, BoxShape>) {
				OBB box;
				box.center = Matrix::Transform({}, worldMatrix);
				const Vector3 worldHalfExtents = {
					shape.halfExtents.x * std::abs(transform.scale.x),
					shape.halfExtents.y * std::abs(transform.scale.y),
					shape.halfExtents.z * std::abs(transform.scale.z),
				};
				box.min = worldHalfExtents * -1.0f;
				box.max = worldHalfExtents;
				box.rotate = transform.rotate;
				box.UpdateOrientation();
				MyDebugLine::AddShape(box, color);
			} else if constexpr (std::is_same_v<ShapeType, PlaneShape>) {
				SubmitPlane(shape, worldMatrix, rotationMatrix, color);
			} else if constexpr (std::is_same_v<ShapeType, RingShape>) {
				SubmitCircle(shape.normal, shape.outerRadius, worldMatrix, color);
				SubmitCircle(shape.normal, shape.innerRadius, worldMatrix, color);
				const Vector3 center = Matrix::Transform({}, worldMatrix);
				const Vector3 worldNormal = NormalizeDirection(Matrix::Transform(shape.normal, rotationMatrix));
				SubmitSegment(center, center + worldNormal * kNormalLineLength, kNormalColor);
			}
		}, config.shape);
	}

} // namespace MadoEngine::Particle
