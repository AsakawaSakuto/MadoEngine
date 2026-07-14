#include "WeaponIconUI.h"
#include "GameObject/Weapon/WeaponInventory.h"

namespace Weapon {

	void WeaponIconUI::Initialize(int slotCount) {
	
		weaponIcons_.resize(slotCount);
		weaponFrames_.resize(slotCount);

		for (int i = 0; i < slotCount; i++) {

			weaponFrames_[i] = MySprite::Create("weaponFrame" + std::to_string(i), "IconFrame", SceneType::Test);
			weaponFrames_[i]->SetPosition(Vector2{ 100.0f + i * 68.0f, 200.0f });

			weaponIcons_[i] = MySprite::Create("weaponIcon" + std::to_string(i), "None", SceneType::Test);
			weaponIcons_[i]->SetPosition(Vector2{ 100.0f + i * 68.0f, 200.0f });
		}
	
	}

	void WeaponIconUI::Update(const Inventory& inventory) {
		for (std::size_t slotIndex = 0; slotIndex < weaponIcons_.size(); ++slotIndex) {
			

			const BaseWeapon* weapon = inventory.GetWeaponAtSlot(slotIndex);
			const Projectile::Type weaponType = weapon ? weapon->GetProjectileType() : Projectile::Type::None;
			const std::string textureName = Projectile::ProjectileTypeToString(weaponType);

			if (weaponIcons_[slotIndex]->GetTextureName() != textureName) {
				weaponIcons_[slotIndex]->SetTexture(textureName);
			}
		}
	}
}
