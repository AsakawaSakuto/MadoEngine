#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MapBlock.h"

/// @brief マップ全体を管理するクラスです。
class Map {
public:
	/// @brief マップを初期化します。
	void Initialize();

	/// @brief マップを更新します。
	void Update();

	/// @brief マップ調整用のImGuiを描画します。
	void DrawImGui();

private:
	/// @brief 地形を再生成します。
	void RegenerateTerrain();

	/// @brief 地形生成用の高さ設定を有効範囲に補正します。
	void ClampHeightSettings();

	/// @brief 指定座標のブロック高さを取得します。
	/// @param x マップ上のX座標です。
	/// @param z マップ上のZ座標です。
	/// @return ブロック高さです。
	uint32_t GetBlockHeight(int x, int z) const;

	std::vector<std::vector<MapBlock>> mapBlocks_;

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

	float slopeSpawnRate_ = 1.0f;
};