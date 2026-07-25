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
		float damage = 0.0f;
		bool wasKilled = false;
	};

	/// @brief 生成されたEnemyの所有と一括処理を管理するクラス
	class Manager {
	public:
		/// @brief Enemy::Managerを初期化する
		/// @param player Enemyとの相互作用に使用するPlayer
		void Initialize(Player::Base* player);

		/// @brief Enemyを生成して管理対象へ登録する
		/// @param desc Enemyの生成情報
		void Spawn(const SpawnDesc& desc);

		/// @brief 管理中のEnemyを一括更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Collider更新後にEnemyの衝突とダメージを一括解決する
		void ResolveAfterCollision();

		/// @brief 管理中のEnemyのColliderをDebugLineへ登録する
		void DrawDebugLine() const;

		/// @brief 管理中のEnemyをすべて破棄する
		void Clear();

		/// @brief 現在管理しているEnemy数を取得する
		/// @return 現在管理しているEnemy数
		std::size_t GetEnemyCount() const { return enemies_.size(); }

		/// @brief Playerに最も近いEnemyの座標を取得する
		/// @param outPosition 最も近いEnemyの座標を受け取る変数
		/// @return 取得できた場合はtrueを返す
		bool TryGetNearestEnemyPosition(Vector3& outPosition) const;

		/// @brief Playerに最も近いEnemyの座標を取得する
		/// @return 最も近いEnemyの座標。Enemyが存在しない場合はゼロ座標を返す
		Vector3 GetNearestEnemyPosition() const;

		/// @brief 未処理のProjectileダメージイベントを取得してキューを空にする
		/// @return 発生順に格納されたProjectileダメージイベント
		std::vector<ProjectileDamageEvent> ConsumeProjectileDamageEvents();

	private:
		/// @brief ProjectileとEnemyの衝突結果を処理する
		void ProcessProjectileHits();

		/// @brief PlayerとEnemyの接触結果を処理する
		void ProcessPlayerCollisions();

		/// @brief 無効になったEnemyを管理対象から削除する
		void RemoveInactiveEnemies();

		Player::Base* player_ = nullptr;
		std::vector<std::unique_ptr<Base>> enemies_;
		std::vector<ProjectileDamageEvent> projectileDamageEvents_;
		std::uint32_t nextEnemyId_ = 0;
	};
} // namespace Enemy
