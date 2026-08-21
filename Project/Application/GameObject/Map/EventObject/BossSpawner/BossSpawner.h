#pragma once
#include "../MapEventObjectBase.h"

/// @brief Bossを生成するMapイベントオブジェクト
class BossSpawner : public MapEventObjectBase {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		std::string colliderName = "BossSpawnerAABB";
	};

	/// @brief BossSpawnerのリソースを解放
	~BossSpawner() override;

	/// @brief 設定付きでBossSpawnerを初期化
	/// @param desc 初期化に使用する設定
	void Initialize(const InitializeDesc& desc);

	/// @brief 指定座標に配置するBossSpawnerのColliderを作成
	/// @param position 配置予定のワールド座標
	/// @return ワールド座標反映済みCollider
	static AABB CreatePlacementCollider(const Vector3& position);

	/// @brief BossSpawnerを更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief BossSpawnerとの相互作用を処理
	/// @param player 相互作用したPlayer
	/// @return BossSpawnerをMapから削除するためtrue
	bool Interact(Player::Base& player) override;

	/// @brief BossSpawnerの操作案内文を取得
	/// @return 操作案内に表示するUTF-8文字列
	std::string_view GetInteractionText() const override;

	/// @brief Playerの初期配置を妨げるObjectか判定
	/// @return Playerの初期配置を妨げるためtrue
	bool ShouldBlockPlayerSpawn() const override { return true; }

	/// @brief Boss生成用のMapイベント要求を取得
	/// @return BossSpawner上端を生成座標にしたBoss生成要求
	MapEventRequest GetInteractionRequest() const override;
};
