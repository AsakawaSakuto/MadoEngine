#pragma once
#include "Entity/Pistol.h"
#include "Entity/Rock.h"
#include "Entity/Axe.h"
#include "Entity/FireBall.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Projectile {
	/// @brief Enemyとの衝突時に渡すProjectileの攻撃情報
	struct HitInfo {
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

		/// @brief 指定コライダーに接触しているProjectileの攻撃情報を収集
		/// @param targetColliderName 接触対象のコライダー名
		/// @param outHitInfos 接触中のProjectile攻撃情報
		void CollectHitsAgainst(const std::string& targetColliderName, std::vector<HitInfo>& outHitInfos) const;

	private:
		std::vector<std::unique_ptr<IProjectile>> projectiles;
		std::uint64_t nextProjectileId_ = 1;
	};
}
