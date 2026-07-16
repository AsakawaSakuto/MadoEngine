#pragma once
#include "Entity/Explosion.h"
#include "Entity/Pistol.h"
#include "Entity/Rock.h"
#include "Entity/Axe.h"
#include "Entity/FireBall.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Projectile {
	/// @brief Projectileとの衝突判定に使用するEnemy情報
	struct EnemyTargetInfo {
		std::uint32_t enemyId = 0;
		std::string colliderName;
		Vector3 position = { 0.0f, 0.0f, 0.0f };
	};

	/// @brief Enemyとの衝突時に渡すProjectileの攻撃情報
	struct HitInfo {
		std::uint32_t enemyId = 0;
		std::uint64_t projectileId = 0;
		float damage = 0.0f;
	};

	class Manager {
	public:
		/// @brief ProjectileManagerのインスタンスを取得
		/// @return ProjectileManagerのインスタンス
		static Manager& GetInstance();

		/// @brief すべてのProjectileを更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Projectileを追加
		/// @param type 追加するProjectileの種類
		/// @param context Projectileの初期化情報
		void AddProjectile(Projectile::Type type, InitializeDesc context);

		/// @brief Enemyに接触しているProjectileの攻撃情報を収集して衝突時の挙動を更新
		/// @param enemyTargets 衝突対象となるEnemy情報
		/// @param outHitInfos 新しく接触したProjectileの攻撃情報
		void CollectHitsAgainst(
			const std::vector<EnemyTargetInfo>& enemyTargets,
			std::vector<HitInfo>& outHitInfos);

	private:
		struct ProjectileAddRequest {
			Projectile::Type type = Projectile::Type::None;
			InitializeDesc context;
		};

		/// @brief Projectileを即時追加
		/// @param type 追加するProjectileの種類
		/// @param context Projectileの初期化情報
		void AddProjectileImmediate(Projectile::Type type, InitializeDesc context);

		/// @brief 保留中のProjectile追加要求を処理
		void FlushPendingProjectiles();

		std::vector<std::unique_ptr<IProjectile>> projectiles;
		std::vector<ProjectileAddRequest> pendingProjectileAddRequests_;
		std::uint64_t nextProjectileId_ = 1;
		bool isTraversingProjectiles_ = false;
	};
}
