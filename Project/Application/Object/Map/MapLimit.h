#pragma once
#include "../../../Engine/Math/Vector3.h"

/// マップの四辺の制限を表す構造体
struct MapLimit {
	Vector3 min = { -7.5f, 0.0f, -7.5f };     /// マップの最小座標
	Vector3 max = { 292.5f, 100.0f, 292.5f }; /// マップの最大座標
};