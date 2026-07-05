#include "BaseWeapon.h"

namespace Weapon {

	void BaseWeapon::Initialize() {
		// 武器の初期化処理
		defaultStatus_ = DefaultStatus();
		specialStatus_ = SpecialStatus();

		type_ = Type::Pistol;

		upgradeLevel = 0;
		killCount = 0;
		slotIndex = -1;

		intervalTimer_.Start(defaultStatus_.shotInterval, true);
		cooldownTimer_.Start(defaultStatus_.shotCooldown, false);
	}

	void BaseWeapon::Update(float deltaTime) {
		
		CreateProjectile(deltaTime);

	}

	void BaseWeapon::CreateProjectile(float deltaTime) {

		// クールタイムが終了している場合射撃を開始する
		if (cooldownTimer_.IsFinished()) {
			if (!intervalTimer_.IsActive()) {
				intervalTimer_.Start(defaultStatus_.shotInterval, true);
				cooldownTimer_.Reset();
			}
		}

		// 最大数射撃数に達していない場合、射撃間隔が終了したら射撃を行う
		if (intervalTimer_.IsFinished()) {
			defaultStatus_.shotNowCount++;
			projectileCount++;

			Projectile::ProjectileManager::GetInstance().AddProjectile(type_, weaponName_, projectileCount);

			// 最大射撃数に達した場合、射撃数をリセットしてクールダウンを開始する
			if (defaultStatus_.shotNowCount >= static_cast<int>(defaultStatus_.shotMaxCount)) {
				defaultStatus_.shotNowCount = 0;
				intervalTimer_.Reset();
				cooldownTimer_.Start(defaultStatus_.shotCooldown, false);
			}
		}

		intervalTimer_.Update(deltaTime);
		cooldownTimer_.Update(deltaTime);
	}
}