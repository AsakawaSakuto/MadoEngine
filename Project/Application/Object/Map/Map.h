#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MapBlockType.h"

/// @brief マップクラス サイズ20x20
class Map {
public:
	void Initialize();

	/// @brief マップが登録したリソースとColliderを破棄する
	void Finalize();

	void Update();

	void DrawImGui();

private:
	/// @brief 地形を再生成する
	void RegenerateTerrain();

	/// @brief 生成済みの地形オブジェクトを破棄する
	void ClearTerrainObjects();

	/// @brief 地形生成用の高さ設定を有効な範囲に補正する
	void ClampHeightSettings();

	std::vector<std::vector<uint32_t>> mapPositionY_;
	std::vector<std::vector<MapBlockType>> mapBlockType_;
	std::vector<std::vector<Vector3>> mapTranslate_;
	std::vector<std::vector<Shape>> mapShape_;
	std::vector<std::vector<Model*>> mapModel_;
	std::vector<std::vector<Model*>> mapSlopeModel_;

	int mapWidth_ = 20;
	int mapHeight_ = 20;

	bool isModelDraw_ = true;

	Vector3 blockSize_ = { 15.0f, 7.5f, 15.0f };

	int minHeight_ = 1;
	int maxHeight_ = 10;

	int minStartHeight_ = 1;
	int maxStartHeight_ = 2;

	int minRangeHeight_ = -1;
	int maxRangeHeight_ = 1;
};
