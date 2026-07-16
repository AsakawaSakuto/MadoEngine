#pragma once
#include "EnemyManager.h"
#include "GameObject/Map/MapLimit.h"
#include <cstddef>

namespace Player {
	class Base;
}

namespace Enemy {

	/// @brief 時間経過に応じてEnemyの生成要求を発行するクラス
	class Spawner {
	public:
		/// @brief Enemy::Spawnerを初期化する
		/// @param player 生成位置の基準になるPlayer
		/// @param enemyManager Enemyの生成要求を登録するManager
		/// @param sceneType Enemyを所属させるシーン種別
		void Initialize(Player::Base* player, Manager* enemyManager, SceneType sceneType);

		/// @brief 経過時間を更新し、生成条件を満たしたEnemyをManagerへ登録する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Enemy::Spawnerのデバッグ用ImGuiを描画する
		void DrawImGui();

		/// @brief 生成時間と強化時間を初期化する
		void Clear();

	private:
		/// @brief Enemyの生成要求を1件発行する
		void SpawnEnemy();

		/// @brief Player周辺の生成位置を作成する
		/// @return Enemyの生成位置
		Vector3 CreateSpawnPosition() const;

		/// @brief 現在の経過時間に応じたEnemyステータスを計算する
		/// @return 新しく生成するEnemyのステータス
		Data::Status CalculateSpawnStatus() const;

		Player::Base* player_ = nullptr;
		Manager* enemyManager_ = nullptr;
		SceneType sceneType_ = SceneType::None;
		MapLimit mapLimit_;
		Data::Status baseStatus_;
		std::size_t spawnLimit_ = 500;
		float spawnInterval_ = 10.0f;
		float spawnTimer_ = 0.0f;
		float elapsedTime_ = 0.0f;
		float minSpawnRadius_ = 8.0f;
		float maxSpawnRadius_ = 14.0f;
		float healthPowerGrowthRatePerMinute_ = 0.1f;
		float moveSpeedGrowthRatePerMinute_ = 0.02f;
		bool isActive_ = true;
	};
} // namespace Enemy
