#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MapBlock.h"
#include "EventObject/MapEventObjectBase.h"
#include <memory>
#include <vector>

namespace Player {
	class Base;
}

/// @brief Map全体を管理するクラス
class Map {
public:
	/// @brief 指定シードでMapを初期化
	/// @param seed Map生成に使用するシード値
	void Initialize(uint32_t seed);

	/// @brief Mapを更新
	/// @param player 相互作用の対象になるPlayer
	void Update(Player::Base& player);

	/// @brief Map調整用のImGuiを描画
	void DrawImGui();

	/// @brief Map生成に使用したシード値を取得
	/// @return uint32_t Map生成に使用したシード値
	uint32_t GetSeed() const;

private:
	
	/// @brief Map上にJarをランダム配置
	void GenerateJars();

	/// @brief Map上にChestをランダム配置
	void GenerateChests();

	/// @brief Map上のイベントオブジェクトを更新
	/// @param player 相互作用するPlayer
	void UpdateEventObjects(Player::Base& player);

	/// @brief Playerとイベントオブジェクトの相互作用を処理
	/// @param player 相互作用するPlayerです。
	void HandleEventObjectInteraction(Player::Base& player);

	/// @brief 地形生成用の高さ設定を有効範囲に補正
	void ClampHeightSettings();

	/// @brief 指定座標のブロック高さを取得
	/// @param x Map上のX座標
	/// @param z Map上のZ座標
	/// @return ブロックの高さ
	uint32_t GetBlockHeight(int x, int z) const;

	std::vector<std::vector<MapBlock>> mapBlocks_;
	std::vector<std::unique_ptr<MapEventObjectBase>> eventObjects_;
	MapEventObjectBase* currentHitEventObject_ = nullptr;
	Random terrainRandom_;
	Random eventObjectRandom_;
	uint32_t seed_ = 0;

	int mapWidth_ = 20;
	int mapHeight_ = 20;
	int jarSpawnCount_ = 100;
	int chestSpawnCount_ = 50;

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
