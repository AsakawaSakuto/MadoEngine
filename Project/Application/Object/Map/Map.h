#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MapBlockType.h"

/// @brief マップクラス サイズ20x20
class Map {
public:
	void Initialize();

	void Update();

private:
	std::vector<std::vector<uint32_t>> mapPositionY_;
	std::vector<std::vector<Vector3>> mapTranslate_;
	std::vector<std::vector<Shape>> mapShape_;

	int mapWidth_ = 20;
	int mapHeight_ = 20;
};