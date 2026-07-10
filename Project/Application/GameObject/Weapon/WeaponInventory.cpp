#include "WeaponInventory.h"
#include "Projectile/ProjectileManager.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI
#include <cstddef>
#include <string>

namespace Weapon {

#ifdef USE_IMGUI
	namespace {
		constexpr Projectile::Type kSelectableWeaponTypes[] = {
			Projectile::Type::Pistol,
			Projectile::Type::Rock,
			Projectile::Type::FireBall,
			Projectile::Type::Axe,
		};
	}
#endif // USE_IMGUI
	
	void Inventory::Initialize(Projectile::Type type) {
		weapons_.clear();
		weapons_.resize(slotCount_);

		AddWeapon(type);
	}

	void Inventory::Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {
		for (auto& weapon : weapons_) {
			if (weapon) {
				weapon->Update(deltaTime, ownerPosition, targetPosition);
			}
		}
	}

	void Inventory::AddWeapon(Projectile::Type type) {
		if (type == Projectile::Type::None) {
			return;
		}

		if (weapons_.size() < static_cast<std::size_t>(slotCount_)) {
			weapons_.resize(slotCount_);
		}

		for (std::size_t slotIndex = 0; slotIndex < weapons_.size(); ++slotIndex) {
			if (!weapons_[slotIndex] || weapons_[slotIndex]->GetProjectileType() == Projectile::Type::None) {
				weapons_[slotIndex] = std::make_unique<BaseWeapon>();
				weapons_[slotIndex]->Initialize(type, static_cast<int>(slotIndex));
				return;
			}
		}
	}

	void Inventory::RemoveWeapon(int slotIndex) {
		if (slotIndex < 0) {
			return;
		}

		const std::size_t weaponIndex = static_cast<std::size_t>(slotIndex);
		if (weaponIndex >= weapons_.size()) {
			return;
		}

		weapons_[weaponIndex].reset();
	}

	void Inventory::DrawImGui() {

#ifdef USE_IMGUI

		if (weapons_.size() < static_cast<std::size_t>(slotCount_)) {
			weapons_.resize(slotCount_);
		}

		int weaponCount = 0;
		for (const auto& weapon : weapons_) {
			if (weapon && weapon->GetProjectileType() != Projectile::Type::None) {
				weaponCount++;
			}
		}

		ImGui::Begin("武器インベントリ");

		ImGui::Text("武器スロット: %d / %d", weaponCount, slotCount_);
		ImGui::Separator();

		const std::string selectedWeaponName = ProjectileTypeToString(selectedAddWeaponType_);
		if (ImGui::BeginCombo("追加する武器", selectedWeaponName.c_str())) {
			for (const Projectile::Type weaponType : kSelectableWeaponTypes) {
				const bool isSelected = selectedAddWeaponType_ == weaponType;
				const std::string weaponName = ProjectileTypeToString(weaponType);
				if (ImGui::Selectable(weaponName.c_str(), isSelected)) {
					selectedAddWeaponType_ = weaponType;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		const bool canAddWeapon = weaponCount < slotCount_;
		if (!canAddWeapon) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("武器を追加")) {
			AddWeapon(selectedAddWeaponType_);
		}
		if (!canAddWeapon) {
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::TextDisabled("空きスロットがありません");
		}

		ImGui::Separator();

		for (int slotIndex = 0; slotIndex < slotCount_; ++slotIndex) {
			ImGui::PushID(slotIndex);

			const BaseWeapon* weapon = nullptr;
			if (static_cast<std::size_t>(slotIndex) < weapons_.size()) {
				weapon = weapons_[slotIndex].get();
			}

			if (weapon && weapon->GetProjectileType() != Projectile::Type::None) {
				const std::string weaponName = ProjectileTypeToString(weapon->GetProjectileType());
				ImGui::Text("スロット %d: %s  Lv %d  Kill %d",
					slotIndex + 1,
					weaponName.c_str(),
					weapon->GetUpgradeLevel(),
					weapon->GetKillCount());
				ImGui::SameLine();
				if (ImGui::SmallButton("削除")) {
					RemoveWeapon(slotIndex);
				}
			} else {
				ImGui::Text("スロット %d: 空き", slotIndex + 1);
			}

			ImGui::PopID();
		}

		ImGui::End();

#endif // USE_IMGUI
	}
}
