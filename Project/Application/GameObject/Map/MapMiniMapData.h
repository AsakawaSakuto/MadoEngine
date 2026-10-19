#pragma once
#include "MapBlockType.h"
#include "MapLimit.h"
#include "Utility/Collider/Shape/SlopeDirection.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

/// @brief ミニマップへ渡す地形セルの情報
struct MapMiniMapCell {
	MapBlockType type = MapBlockType::Air;
	SlopeDirection slopeDirection = SlopeDirection::PulsX;
	std::uint32_t height = 0;
};

/// @brief ミニマップへ表示するイベントObjectの種別
enum class MapMiniMapObjectType : std::size_t {
	BossSpawner,
	Chest,
	Jar,
	Karma,
	Magnet,
	Anvil,
	Count,
};

/// @brief イベントObjectの種別順に保持するワールド座標一覧
using MapMiniMapObjectPositions = std::array<std::vector<Vector3>,
	static_cast<std::size_t>(MapMiniMapObjectType::Count)>;

/// @brief 描画リソースから独立した生成済みMapのミニマップ情報
struct MapMiniMapData {
	MapLimit limit;
	std::uint32_t width = 0;
	std::uint32_t depth = 0;
	std::uint32_t minHeight = 0;
	std::uint32_t maxHeight = 0;
	std::vector<MapMiniMapCell> cells;
	MapMiniMapObjectPositions objectPositions;
};
