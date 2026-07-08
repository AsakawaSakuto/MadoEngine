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
	/// @brief EnemySpawnerを初期化します。
	/// @param player 生成と追跡の基準になるPlayerです。
	/// @param sceneType Enemyを所属させるシーン種別です。
	void Initialize(Player::Base* player, SceneType sceneType);

	/// @brief Enemyの生成と更新を行います。
	/// @param deltaTime デルタタイムです。
	void Update(float deltaTime);

	/// @brief すべてのEnemyを削除します。
	void Clear();

	/// @brief 現在のEnemy数を取得します。
	/// @return 生存しているEnemy数です。
	std::size_t GetEnemyCount() const { return enemies_.size(); }

	/// @brief Playerに最も近いEnemyの座標を取得します。
	/// @param outPosition 最も近いEnemyの座標を受け取る変数です。
	/// @return 取得できた場合はtrueを返します。
	bool TryGetNearestEnemyPosition(Vector3& outPosition) const;

	/// @brief Playerに最も近いEnemyの座標を取得します。
	/// @return 最も近いEnemyの座標です。Enemyが存在しない場合はゼロ座標を返します。
	Vector3 GetNearestEnemyPosition() const;

private:
	/// @brief Enemyを1体生成します。
	void SpawnEnemy();

	/// @brief Player周辺の生成位置を作成します。
	/// @return Enemyの生成位置です
	Vector3 CreateSpawnPosition() const;

	/// @brief 削除対象になったEnemyを破棄します
	void RemoveDeadEnemies();

	Player::Base* player_ = nullptr;
	SceneType sceneType_ = SceneType::None;
	std::vector<std::unique_ptr<Enemy>> enemies_;
	float spawnInterval_ = 0.1f;
	float spawnTimer_ = 0.0f;
	float minSpawnRadius_ = 8.0f;
	float maxSpawnRadius_ = 14.0f;
	uint32_t nextEnemyId_ = 0;
};
