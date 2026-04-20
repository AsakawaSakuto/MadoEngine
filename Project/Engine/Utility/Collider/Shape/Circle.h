#pragma once
#include "Math/Vector3.h"

// 平円を表す構造体
struct Circle {
	Vector3 center; // 平円の中心座標
	float radius;   // 平円の半径
	Vector3 normal; // 平円の法線
};