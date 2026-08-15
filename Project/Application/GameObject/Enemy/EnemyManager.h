#pragma once
#include "EnemyBase.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Player {
	class Base;
}

namespace Enemy {

	/// @brief Projectileによるダメージ表示へ渡すイベント情報
	struct ProjectileDamageEvent {
		Vector3 worldPosition = { 0.0f, 0.0f, 0.0f };
		std::uint64_t sourceWeaponId = 0;
		float damage = 0.0f;
		bool wasKilled = false;
	};

	/// @brief 生成されたEnemyの所有と一括処理を管理するクラス
	class Manager {
	public:
		/// @brief Enemy::Managerを初期化
		/// @param player Enemyとの相互作用に使用するPlayer
		void Initialize(Player::Base* player);

		/// @brief Enemyを生成して管理対象へ登録
		/// @param desc Enemyの生成情報
		void Spawn(const SpawnDesc& desc);

		/// @brief 指定座標へBossを生成して管理対象へ登録
		/// @param position Bossの生成座標
		/// @param sceneType Bossを所属させるシーン種別
		void SpawnBoss(const Vector3& position, SceneType sceneType);

		/// @brief 管理中のEnemyを一括更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Collider更新後にEnemyの衝突とダメージを一括解決
		void ResolveAfterCollision();

		/// @brief 管理中のEnemyのColliderをDebugLineへ登録
		void DrawDebugLine() const;

		/// @brief 管理中のEnemyをすべて破棄
		void Clear();

		/// @brief 現在管理しているEnemy数を取得
		/// @return 現在管理しているEnemy数
		std::size_t GetEnemyCount() const { return enemies_.size(); }

		/// @brief Playerに最も近いEnemyの座標を取得
		/// @param outPosition 最も近いEnemyの座標を受け取る変数
		/// @return 取得できた場合はtrue
		bool TryGetNearestEnemyPosition(Vector3& outPosition) const;

		/// @brief Playerに最も近いEnemyの座標を取得
		/// @return 最も近いEnemyの座標、Enemyが存在しない場合はゼロ座標
		Vector3 GetNearestEnemyPosition() const;

		/// @brief 未処理のProjectileダメージイベントを取得してキューをクリア
		/// @return 発生順に格納されたProjectileダメージイベント
		std::vector<ProjectileDamageEvent> ConsumeProjectileDamageEvents();

	private:
		/// @brief ProjectileとEnemyの衝突結果を処理
		void ProcessProjectileHits();

		/// @brief PlayerとEnemyの接触結果を処理
		void ProcessPlayerCollisions();

		/// @brief Player周辺の削除範囲外にいるEnemyを無効化
		void DeactivateEnemiesOutsidePlayerRange();

		/// @brief 無効になったEnemyを管理対象から削除
		void RemoveInactiveEnemies();

		Player::Base* player_ = nullptr;
		std::vector<std::unique_ptr<Base>> enemies_;
		std::vector<ProjectileDamageEvent> projectileDamageEvents_;
		std::uint32_t nextEnemyId_ = 0;
	};
} // namespace Enemy
