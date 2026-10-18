#pragma once
#include "MapGenerationSettings.h"
#include "MapBlock.h"
#include "EventObject/MapEventObjectBase.h"
#include "Utility/Random.h"
#include <memory>
#include <vector>

/// @brief 生成済み地形へのイベントオブジェクト配置を担当するクラス
class MapEventObjectSpawner final {
public:
	/// @brief 一回の生成に使用する設定と配置先を接続
	/// @param settings 適用済みの生成設定
	/// @param blocks 配置先の読み取り専用地形
	/// @param eventObjects 生成したイベントオブジェクトの格納先
	MapEventObjectSpawner(const MapGenerationSettings& settings,
		const std::vector<std::vector<MapBlock>>& blocks,
		std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects);

	/// @brief 指定シードの専用乱数系列で全種類のイベントを配置
	void Generate();

private:
	/// @brief 占有幅を確保した地形上の配置候補を作成
	/// @param localCollider 原点を基準にした配置用Collider
	/// @param outPosition 地表面に接地した候補座標
	/// @param outBlock 配置候補のBlockを受け取るポインタ
	/// @return 有効な地形上の候補を作成した場合はtrue
	bool TryCreateSpawnPosition(const AABB& localCollider, Vector3& outPosition, const MapBlock*& outBlock);

	/// @brief 配置候補と既存イベントの重複を判定
	/// @param collider 配置候補のワールド座標反映済みCollider
	/// @return 既存Colliderと重なる場合はtrue
	bool IsEventObjectColliderOverlapping(const AABB& collider) const;

	/// @brief Jarをランダム配置
	void GenerateJars();

	/// @brief Chestをランダム配置
	void GenerateChests();

	/// @brief Karmaをランダム配置
	void GenerateKarmas();

	/// @brief 通常Block上にBossSpawnerを配置
	void GenerateBossSpawner();

	/// @brief Magnetをランダム配置
	void GenerateMagnets();

	/// @brief Anvilをランダム配置
	void GenerateAnvils();

	const MapGenerationSettings& settings_;
	const std::vector<std::vector<MapBlock>>& mapBlocks_;
	std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects_;
	Random eventObjectRandom_;
};
