#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MapBlock.h"
#include "EventObject/MapEventObjectBase.h"
#include <memory>
#include <vector>

class Player;

/// @brief Map全体を管理するクラスです。
class Map {
public:
	/// @brief Mapを初期化します。
	void Initialize();

	/// @brief Mapを更新します。
	/// @param player 相互作用の対象になるPlayerです。
	void Update(Player& player);

	/// @brief Map調整用のImGuiを描画します。
	void DrawImGui();

private:
	/// @brief 地形を再生成します。
	void RegenerateTerrain();

	/// @brief Map上にJarをランダム配置します。
	void GenerateJars();

	/// @brief Map上のイベントオブジェクトを更新します。
	/// @param player 相互作用するPlayerです。
	void UpdateEventObjects(Player& player);

	/// @brief Playerとイベントオブジェクトの相互作用を処理します。
	/// @param player 相互作用するPlayerです。
	void HandleEventObjectInteraction(Player& player);

	/// @brief 地形生成用の高さ設定を有効範囲に補正します。
	void ClampHeightSettings();

	/// @brief 指定座標のブロック高さを取得します。
	/// @param x Map上のX座標です。
	/// @param z Map上のZ座標です。
	/// @return ブロックの高さです。
	uint32_t GetBlockHeight(int x, int z) const;

	std::vector<std::vector<MapBlock>> mapBlocks_;
	std::vector<std::unique_ptr<MapEventObjectBase>> eventObjects_;
	MapEventObjectBase* currentHitEventObject_ = nullptr;

	int mapWidth_ = 20;
	int mapHeight_ = 20;
	int jarSpawnCount_ = 100;

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
