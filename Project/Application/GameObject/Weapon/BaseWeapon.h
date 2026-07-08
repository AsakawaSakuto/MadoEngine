#pragma once
#include "UtilityHeaders.h"
#include "Projectile/ProjectileManager.h"
#include "Projectile/ProjectileStatus.h"
#include <string>

namespace Weapon {
	
	class BaseWeapon {
	public:
		void Initialize(Projectile::Type type, int slotIndex);

		void Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		void CreateProjectile(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		Projectile::Type GetProjectileType() const { return type_; }

		int GetUpgradeLevel() const { return upgradeLevel_; }

		int GetKillCount() const { return killCount_; }

	private:
		// 武器のステータス
		DefaultStatus defaultStatus_;
		SpecialStatus specialStatus_;

		// 武器の種類
		Projectile::Type type_ = Projectile::Type::None;

		int killCount_ = 0;        // 武器の総キル数
		float damageCount_ = 0.0f; // 武器の総ダメージ量

		int upgradeLevel_ = 0;    // 武器のアップグレードレベル
		int slotIndex_ = -1;      // 武器のスロットインデックス
		int projectileCount_ = 0; // 武器の発射数

		GameTimer intervalTimer_;
		GameTimer cooldownTimer_;

		std::string weaponName_;
	};
}