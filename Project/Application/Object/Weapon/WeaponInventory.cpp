#include "WeaponInventory.h"
#include "Projectile/ProjectileManager.h"
#include <cstddef>

namespace Weapon {
	
	void Inventory::Initialize(Type type) {
		weapons_.resize(slotCount_);
		
		weapons_[0] = std::make_unique<BaseWeapon>();
		weapons_[0]->Initialize(type, 0);
	}

	void Inventory::Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {
		for (auto& weapon : weapons_) {
			if (weapon) {
				weapon->Update(deltaTime, ownerPosition, targetPosition);
			}
		}
	}

	void Inventory::AddWeapon(Type type) {
		if (weapons_.size() < static_cast<std::size_t>(slotCount_)) {
			weapons_.resize(slotCount_);
		}

		for (std::size_t slotIndex = 0; slotIndex < weapons_.size(); ++slotIndex) {
			if (!weapons_[slotIndex] || weapons_[slotIndex]->GetType() == Type::None) {
				weapons_[slotIndex] = std::make_unique<BaseWeapon>();
				weapons_[slotIndex]->Initialize(type, static_cast<int>(slotIndex));
				return;
			}
		}
	}
}
