#include "WeaponInventory.h"
#include "Projectile/ProjectileManager.h"

namespace Weapon {
	
	void Inventory::Initialize() {
		weapons_.resize(slotCount_);
		for (int i = 0; i < slotCount_; ++i) {
			weapons_[i] = std::make_unique<BaseWeapon>();
			weapons_[i]->Initialize();
		}
	}

	void Inventory::Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {
		for (auto& weapon : weapons_) {
			if (weapon) {
				weapon->Update(deltaTime, ownerPosition, targetPosition);
			}
		}
	}
}
