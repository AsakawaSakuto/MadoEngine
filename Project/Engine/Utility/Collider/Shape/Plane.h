#pragma once
#include "../../../Math/Vector3.h"

// 平面を表す構造体
struct Plane {
	Vector3 center; // 平面の中心座標
	Vector3 normal; // 平面の法線
	float size; 
};