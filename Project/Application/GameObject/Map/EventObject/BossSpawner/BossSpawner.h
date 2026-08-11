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

	/// @brief BossSpawnerを更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief BossSpawnerとの相互作用を処理
	/// @param player 相互作用したPlayer
	/// @return BossSpawnerをMap上へ維持するためfalse
	bool Interact(Player::Base& player) override;
};
