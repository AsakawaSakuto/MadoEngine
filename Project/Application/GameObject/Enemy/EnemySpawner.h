#pragma once
#include "Enemy.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../DropObject/DropObjectManager.h"

namespace Player {
	class Base;
}

class EnemySpawner {
public:
	/// @brief EnemySpawnerを初期化
	/// @param player 生成と追跡の基準になるPlayer
	/// @param sceneType Enemyを所属させるシーン種別
	void Initialize(Player::Base* player, SceneType sceneType);

	/// @brief Enemyの生成と更新を行う
	/// @param deltaTime デルタタイム
	void Update(float deltaTime);

	/// @brief EnemySpawnerのデバッグ用ImGuiを描画
	void DrawImGui();

	/// @brief すべてのEnemyを削除
	void Clear();

	/// @brief 現在のEnemy数を取得
	/// @return 生存しているEnemy数
	std::size_t GetEnemyCount() const { return enemies_.size(); }

	/// @brief Playerに最も近いEnemyの座標を取得
	/// @param outPosition 最も近いEnemyの座標を受け取る変数
	/// @return 取得できた場合はtrueを返す
	bool TryGetNearestEnemyPosition(Vector3& outPosition) const;

	/// @brief Playerに最も近いEnemyの座標を取得
	/// @return 最も近いEnemyの座標。Enemyが存在しない場合はゼロ座標を返す
	Vector3 GetNearestEnemyPosition() const;

private:
	/// @brief Enemyを1体生成
	void SpawnEnemy();

	/// @brief Player周辺の生成位置を作成
	/// @return Enemyの生成位置
	Vector3 CreateSpawnPosition() const;

	/// @brief 削除対象になったEnemyを破棄
	void RemoveDeadEnemies();

	Player::Base* player_ = nullptr;
	SceneType sceneType_ = SceneType::None;
	std::vector<std::unique_ptr<Enemy>> enemies_;

	int spawnLimit_ = 499;

	float spawnInterval_ = 10.0f;
	float spawnTimer_ = 0.0f;
	float minSpawnRadius_ = 8.0f;
	float maxSpawnRadius_ = 14.0f;
	uint32_t nextEnemyId_ = 0;
	bool isActive_ = true;
};
