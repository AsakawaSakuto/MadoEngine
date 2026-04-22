#pragma once
#include "Math/Vector3.h"

/// @brief フラスタム平面構造体
struct FrustumPlane {
	Vector3 normal;
	float distance;
};

/// @brief フラスタム構造体
struct Frustum {
	FrustumPlane planes[6]; // 6つの平面（左、右、上、下、近、遠）
};