#include "BaseWeapon.h"

namespace Weapon {

	void BaseWeapon::Initialize(Projectile::Type type, int slotIndex) {
		slotIndex_ = slotIndex;
		type_ = type;
		weaponName_ = ProjectileTypeToString(type_);

		upgradeLevel_ = 0;
		killCount_ = 0;
		projectileCount_ = 0;

		intervalTimer_.Start(shotIntervalTime_, true);
		cooldownTimer_.Start(status_.shotCooldown.value, false);
	}

	void BaseWeapon::Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {
		
		CreateProjectile(deltaTime, ownerPosition, targetPosition);

	}

	void BaseWeapon::CreateProjectile(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {

		// クールタイムが終了している場合射撃を開始する
		if (cooldownTimer_.IsFinished()) {
			if (!intervalTimer_.IsActive()) {
				intervalTimer_.Start(shotIntervalTime_, true);
				cooldownTimer_.Reset();
			}
		}

		// 最大数射撃数に達していない場合、射撃間隔が終了したら射撃を行う
		if (intervalTimer_.IsFinished()) {
			shotNowCount_++;
			projectileCount_++;

			Projectile::InitializeDesc context;
			context.projectileName = weaponName_;
			context.projectileCount = projectileCount_;
			context.ownerPosition = ownerPosition;
			context.targetPosition = targetPosition;

			Projectile::Manager::GetInstance().AddProjectile(type_, context);

			// 最大射撃数に達した場合、射撃数をリセットしてクールダウンを開始する
			if (shotNowCount_ >= static_cast<int>(status_.shotMaxCount.value)) {
				shotNowCount_ = 0;
				intervalTimer_.Reset();
				cooldownTimer_.Start(status_.shotCooldown.value, false);
			}
		}

		intervalTimer_.Update(deltaTime);
		cooldownTimer_.Update(deltaTime);
	}
}
