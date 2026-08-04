#include "WeaponIconUI.h"
#include "GameObject/Weapon/WeaponInventory.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>

namespace UI::Game {

	void WeaponIconUI::Initialize(int slotCount) {
	
		weaponIcons_.resize(slotCount);
		weaponIconsBG_.resize(slotCount);
		weaponIconFrames_.resize(slotCount);
		weaponIconsShotGauge_.resize(slotCount);
		animationStates_.resize(slotCount);

		for (int i = 0; i < slotCount; i++) {

			weaponIconsBG_[i] = MySprite::Create("weaponIconBG" + std::to_string(i), "white2x2", SceneType::Game);
			weaponIconsBG_[i]->SetPosition(Vector2{ 100.0f + i * 68.0f, 200.0f - 64.0f });
			weaponIconsBG_[i]->SetScale(Vector2{ 32.0f, 32.0f });
			weaponIconsBG_[i]->SetColor(Vector4{ 0.1f, 0.1f, 0.1f, 1.0f });

			weaponIconsShotGauge_[i] = MySprite::Create("weaponIconShotGauge" + std::to_string(i), "white2x2", SceneType::Game);
			weaponIconsShotGauge_[i]->SetPosition(Vector2{ 104.0f + i * 68.0f, 260.0f - 64.0f });
			weaponIconsShotGauge_[i]->SetScale(Vector2{ shotGaugeSize_, 0.0f });
			weaponIconsShotGauge_[i]->SetColor(Vector4{ 0.5f, 0.5f, 0.5f, 1.0f });
			weaponIconsShotGauge_[i]->SetAnchorPoint(Vector2{ 0.0f, 1.0f });

			weaponIconFrames_[i] = MySprite::Create("weaponFrame" + std::to_string(i), "IconFrame", SceneType::Game);
			weaponIconFrames_[i]->SetPosition(Vector2{ 100.0f + i * 68.0f, 200.0f - 64.0f });

			weaponIcons_[i] = MySprite::Create("weaponIcon" + std::to_string(i), "None", SceneType::Game);
			weaponIcons_[i]->SetPosition(Vector2{ 132.0f + i * 68.0f, 232.0f - 64.0f });
			weaponIcons_[i]->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
		}
	
	}

	void WeaponIconUI::PlayFireAnimation(std::size_t slotIndex) {
		if (slotIndex >= animationStates_.size()) {
			return;
		}

		IconAnimationState& state = animationStates_[slotIndex];
		state.elapsedTime = 0.0f;
		state.isPlaying = true;
	}

	void WeaponIconUI::Update(float deltaTime, const Weapon::Inventory& inventory) {
		for (std::size_t slotIndex = 0; slotIndex < weaponIcons_.size(); ++slotIndex) {
			const Weapon::BaseWeapon* weapon = inventory.GetWeaponAtSlot(slotIndex);
			const Projectile::Type weaponType = weapon ? weapon->GetProjectileType() : Projectile::Type::None;
			const std::string textureName = Projectile::ProjectileTypeToString(weaponType);

			if (weaponIcons_[slotIndex]->GetTextureName() != textureName) {
				weaponIcons_[slotIndex]->SetTexture(textureName);
			}

			const float shotCooldownProgress = weapon
				? std::clamp(weapon->GetShotCooldownProgress(), 0.0f, 1.0f)
				: 0.0f;
			weaponIconsShotGauge_[slotIndex]->SetScale({
				shotGaugeSize_,
				shotGaugeSize_ * shotCooldownProgress,
			});

			if (!weapon) {
				animationStates_[slotIndex] = {};
				weaponIcons_[slotIndex]->SetScale(startIconScale);
				continue;
			}

			UpdateFireAnimation(deltaTime, slotIndex);
		}
	}

	void WeaponIconUI::UpdateFireAnimation(float deltaTime, std::size_t slotIndex) {
		if (slotIndex >= weaponIcons_.size() || slotIndex >= animationStates_.size()) {
			return;
		}

		IconAnimationState& state = animationStates_[slotIndex];
		if (!state.isPlaying) {
			weaponIcons_[slotIndex]->SetScale(startIconScale);
			return;
		}

		state.elapsedTime += (std::max)(deltaTime, 0.0f);
		const float progress = std::clamp(
			state.elapsedTime / fireAnimationDuration_,
			0.0f,
			1.0f);
		const Vector2 scale = Easing::LerpBack(
			startIconScale,
			endIconScale,
			progress,
			EaseType::EaseOutCubic,
			EaseType::EaseInCubic);

		weaponIcons_[slotIndex]->SetScale(scale);
		if (progress >= 1.0f) {
			weaponIcons_[slotIndex]->SetScale(startIconScale);
			state.isPlaying = false;
		}
	}
}
