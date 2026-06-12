#pragma once
#include "Math/Vector3.h"

/// @brief 視錐台を構成する平面
struct FrustumPlane {
	Vector3 normal;
	float distance;
};

/// @brief 視錐台
struct Frustum {
	FrustumPlane planes[6]; // 6つの平面

	/// @brief AABBが視錐台と交差しているか判定する
	/// @param min AABBの最小座標
	/// @param max AABBの最大座標
	/// @return 視錐台内、または視錐台と交差している場合はtrue
	bool IntersectsAABB(const Vector3& min, const Vector3& max) const {
		for (const FrustumPlane& plane : planes) {
			const Vector3 positiveVertex = {
				plane.normal.x >= 0.0f ? max.x : min.x,
				plane.normal.y >= 0.0f ? max.y : min.y,
				plane.normal.z >= 0.0f ? max.z : min.z
			};

			const float distance =
				plane.normal.x * positiveVertex.x +
				plane.normal.y * positiveVertex.y +
				plane.normal.z * positiveVertex.z +
				plane.distance;

			if (distance < 0.0f) {
				return false;
			}
		}

		return true;
	}
};
