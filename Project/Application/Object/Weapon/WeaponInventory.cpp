#include "WeaponInventory.h"

namespace Weapon {
	
	void Inventory::Initialize() {
		weapons_.resize(slotCount_);
		for (int i = 0; i < slotCount_; ++i) {
			weapons_[i] = std::make_unique<BaseWeapon>();
			weapons_[i]->Initialize();
		}
	}

	void Inventory::Update(float deltaTime) {
		for (auto& weapon : weapons_) {
			if (weapon) {
				weapon->Update(deltaTime);
			}
		}
	}
}