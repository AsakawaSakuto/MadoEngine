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
		/// @brief Enemy::Spawnerを初期化
		/// @param player 生成位置の基準になるPlayer
		/// @param enemyManager Enemyの生成要求を登録するManager
		/// @param sceneType Enemyを所属させるシーン種別
		void Initialize(Player::Base* player, Manager* enemyManager, SceneType sceneType);

		/// @brief 経過時間を更新し、生成条件を満たしたEnemyをManagerへ登録
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Enemy::Spawnerのデバッグ用ImGuiを描画
		void DrawImGui();

		/// @brief 生成時間と強化時間を初期化
		void Clear();

		/// @brief Enemy生成位置のMap制限を設定
		/// @param mapLimit Map外周と高さを表す移動制限
		void SetMapLimit(const MapLimit& mapLimit) { mapLimit_ = mapLimit; }

	private:
		/// @brief 通常湧きの抽選率から生成するEnemy種類を選択
		/// @return 生成するEnemyの種類
		Data::Type SelectSpawnType() const;

		/// @brief Enemyの生成要求を1件発行
		void SpawnEnemy();

		/// @brief Player周辺の地表面からEnemyの生成位置を作成
		/// @param outPosition Enemyの生成位置
		/// @param outGroundSurfaceY 生成地点の地表面Y座標
		/// @return 生成可能な地表面が見つかった場合はtrue
		bool TryCreateSpawnPosition(Vector3& outPosition, float& outGroundSurfaceY) const;

		/// @brief Enemy種類と現在の経過時間に応じたステータスを計算
		/// @param type ステータスを計算するEnemyの種類
		/// @return 新しく生成するEnemyのステータス
		Data::Status CalculateSpawnStatus(Data::Type type) const;

		Player::Base* player_ = nullptr;
		Manager* enemyManager_ = nullptr;
		SceneType sceneType_ = SceneType::None;
		MapLimit mapLimit_;
		std::size_t spawnLimit_ = 500;
		float spawnInterval_ = 0.4f;
		float spawnTimer_ = 0.0f;
		float elapsedTime_ = 0.0f;
		float minSpawnRadius_ = 8.0f;
		float maxSpawnRadius_ = 14.0f;
		float runnerSpawnRate_ = 0.25f;
		float tankSpawnRate_ = 0.15f;
		float healthPowerGrowthRatePerMinute_ = 0.1f;
		float moveSpeedGrowthRatePerMinute_ = 0.02f;
		bool isActive_ = true;
	};
} // namespace Enemy
