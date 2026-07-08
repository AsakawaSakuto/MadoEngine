#pragma once

/// @brief Map上のブロックの種類を表す列挙型
enum class MapBlockType {
	Air,    // 空気
	Ground, // 地面
	Slope,  // 坂

	Count
};