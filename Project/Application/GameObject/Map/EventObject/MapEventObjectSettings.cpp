#include "MapEventObjectSettings.h"
#include "Utility/Collider/Shape/AABB.h"
#include <algorithm>
#include <cmath>

void MapEventObjectSettings::Clamp() {

	// 0秒は即時発火として許可し、不正な時間だけ従来の1秒へ復帰
	if (!std::isfinite(autoInteractionDuration)) {
		autoInteractionDuration = 1.0f;
	}
	autoInteractionDuration = (std::max)(0.0f, autoInteractionDuration);

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

bool MapEventObjectSettings::IsEqualTo(const MapEventObjectSettings& rhs) const {
	return autoInteractionDuration == rhs.autoInteractionDuration &&
		colliderMin == rhs.colliderMin && colliderMax == rhs.colliderMax;
}

AABB MapEventObjectSettings::CreateCollider(const Vector3& position) const {
	AABB collider;
	collider.center = position;
	collider.min = colliderMin;
	collider.max = colliderMax;
	return collider;
}
