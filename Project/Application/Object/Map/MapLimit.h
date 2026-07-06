#pragma once

/// マップの四辺の制限を表す構造体
struct MapLimit {
	float minX = -7.5f;  // Mapの最小X座標
	float maxX = 292.5f; // Mapの最大X座標
	float minZ = -7.5f;  // Mapの最小Z座標
	float maxZ = 292.5f; // Mapの最大Z座標
};