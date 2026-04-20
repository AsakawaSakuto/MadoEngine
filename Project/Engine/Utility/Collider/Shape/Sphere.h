#pragma once
#include "Math/Vector3.h"

// 球体
struct Sphere {
	Vector3 center = { 0.0f,0.0f,0.0f }; // 中心点
	float radius = 1.0f;                 // 半径
};