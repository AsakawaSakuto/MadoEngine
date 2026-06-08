#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MapBlockType.h"

/// @brief マップクラス サイズ20x20
class Map {
public:
	void Initialize();

	void Update();

	void DrawImGui();

private:
	std::vector<std::vector<uint32_t>> mapPositionY_;
	std::vector<std::vector<MapBlockType>> mapBlockType_;
	std::vector<std::vector<Vector3>> mapTranslate_;
	std::vector<std::vector<Shape>> mapShape_;
	std::vector<std::vector<Model*>> mapModel_;
	std::vector<std::vector<Model*>> mapSlopeModel_;

	int mapWidth_ = 20;
	int mapHeight_ = 20;

	bool isModelDraw_ = true;

	Vector3 blockSize_ = { 10.0f, 5.0f, 10.0f };

	int minHeight_ = 1;
	int maxHeight_ = 8;

	int minStartHeight_ = 1;
	int maxStartHeight_ = 8;

	int minRangeHeight_ = -1;
	int maxRangeHeight_ = 1;
};
