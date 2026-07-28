#include "WeaponIconUI.h"
#include "GameObject/Weapon/WeaponInventory.h"

namespace UI::Game {

	void WeaponIconUI::Initialize(int slotCount) {
	
		weaponIcons_.resize(slotCount);
		weaponIconsBG_.resize(slotCount);
		weaponIconFrames_.resize(slotCount);

		for (int i = 0; i < slotCount; i++) {

			weaponIconsBG_[i] = MySprite::Create("weaponIconBG" + std::to_string(i), "IconFrameBG", SceneType::Game);
			weaponIconsBG_[i]->SetPosition(Vector2{ 100.0f + i * 68.0f, 200.0f });

			weaponIconFrames_[i] = MySprite::Create("weaponFrame" + std::to_string(i), "IconFrame", SceneType::Game);
			weaponIconFrames_[i]->SetPosition(Vector2{ 100.0f + i * 68.0f, 200.0f });

			weaponIcons_[i] = MySprite::Create("weaponIcon" + std::to_string(i), "None", SceneType::Game);
			weaponIcons_[i]->SetPosition(Vector2{ 132.0f + i * 68.0f, 232.0f });
			weaponIcons_[i]->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
		}
	
	}

	void WeaponIconUI::Update(const Weapon::Inventory& inventory) {
		for (std::size_t slotIndex = 0; slotIndex < weaponIcons_.size(); ++slotIndex) {
			

			const Weapon::BaseWeapon* weapon = inventory.GetWeaponAtSlot(slotIndex);
			const Projectile::Type weaponType = weapon ? weapon->GetProjectileType() : Projectile::Type::None;
			const std::string textureName = Projectile::ProjectileTypeToString(weaponType);

			if (weaponIcons_[slotIndex]->GetTextureName() != textureName) {
				weaponIcons_[slotIndex]->SetTexture(textureName);
			}
		}
	}
}
