#include "MapDecorationObjectSettings.h"
#include "Utility/Collider/Shape/AABB.h"
#include <algorithm>
#include <cmath>

void MapDecorationObjectSettings::Clamp() {

	// 手入力やJson編集による非有限値とMin・Maxの逆転を各軸で補正
	for (std::size_t axis = 0; axis < 3; ++axis) {
		if (!std::isfinite(colliderMin[axis])) {
			colliderMin[axis] = 0.0f;
		}
		if (!std::isfinite(colliderMax[axis])) {
			colliderMax[axis] = 0.0f;
		}
		if (colliderMin[axis] > colliderMax[axis]) {
			std::swap(colliderMin[axis], colliderMax[axis]);
		}
	}
}

bool MapDecorationObjectSettings::IsEqualTo(const MapDecorationObjectSettings& rhs) const {
	return colliderMin == rhs.colliderMin && colliderMax == rhs.colliderMax;
}

AABB MapDecorationObjectSettings::CreateCollider(const Vector3& position) const {
	AABB collider;
	collider.center = position;
	collider.min = colliderMin;
	collider.max = colliderMax;
	return collider;
}
