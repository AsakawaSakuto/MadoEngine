#pragma once
#include "../../../Math/Vector3.h"

// 線分
struct Segment {
	Vector3 origin; // 始点
	Vector3 diff;   // 終点への差分ベクトル
};