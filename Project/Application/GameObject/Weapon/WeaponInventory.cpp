#include "WeaponInventory.h"
#include "Projectile/ProjectileManager.h"
#include "Utility/Logger/Logger.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI
#include <cstddef>
#include <string>

namespace Weapon {
	
	void Inventory::Initialize(Projectile::Type type) {
		weapons_.clear();
		weapons_.resize(slotCount_);
		revision_ = 0;

		if (!AddWeapon(type)) {
			Logger::Output("[Application] 初期武器の追加に失敗しました。", Logger::Level::Error);
		}
	}

	void Inventory::Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition) {
		for (auto& weapon : weapons_) {
			if (weapon) {
				weapon->Update(deltaTime, ownerPosition, targetPosition);
			}
		}
	}

	bool Inventory::AddWeapon(Projectile::Type type) {
		if (!Projectile::IsPlayableWeaponType(type)) {
			Logger::Output("[Application] 無効な武器種類は追加できません。", Logger::Level::Warning);
			return false;
		}

		if (HasWeapon(type)) {
			Logger::Output("[Application] 同じ種類の武器は重複して装備できません: " + ProjectileTypeToString(type), Logger::Level::Warning);
			return false;
		}

		if (!HasEmptySlot()) {
			Logger::Output("[Application] 武器スロットに空きがないため武器を追加できません。", Logger::Level::Warning);
			return false;
		}

		if (weapons_.size() < static_cast<std::size_t>(slotCount_)) {
			weapons_.resize(slotCount_);
		}

		for (std::size_t slotIndex = 0; slotIndex < weapons_.size(); ++slotIndex) {
			if (!weapons_[slotIndex] || weapons_[slotIndex]->GetProjectileType() == Projectile::Type::None) {
				auto weapon = std::make_unique<BaseWeapon>();
				if (!weapon->Initialize(type, static_cast<int>(slotIndex))) {
					Logger::Output("[Application] 武器の初期化に失敗したため追加を中止しました: " + ProjectileTypeToString(type), Logger::Level::Error);
					return false;
				}

				weapons_[slotIndex] = std::move(weapon);
				++revision_;
				return true;
			}
		}

		return false;
	}

	bool Inventory::HasWeapon(Projectile::Type type) const {
		return GetWeapon(type) != nullptr;
	}

	bool Inventory::HasEmptySlot() const {
		if (weapons_.size() < static_cast<std::size_t>(slotCount_)) {
			return true;
		}

		for (const auto& weapon : weapons_) {
			if (!weapon || weapon->GetProjectileType() == Projectile::Type::None) {
				return true;
			}
		}

		return false;
	}

	std::size_t Inventory::GetWeaponCount() const {
		std::size_t weaponCount = 0;
		for (const auto& weapon : weapons_) {
			if (weapon && weapon->GetProjectileType() != Projectile::Type::None) {
				++weaponCount;
			}
		}

		return weaponCount;
	}

	BaseWeapon* Inventory::GetWeapon(Projectile::Type type) {
		return const_cast<BaseWeapon*>(static_cast<const Inventory*>(this)->GetWeapon(type));
	}

	const BaseWeapon* Inventory::GetWeapon(Projectile::Type type) const {
		if (type == Projectile::Type::None) {
			return nullptr;
		}

		for (const auto& weapon : weapons_) {
			if (weapon && weapon->GetProjectileType() == type) {
				return weapon.get();
			}
		}

		return nullptr;
	}

	const BaseWeapon* Inventory::GetWeaponAtSlot(std::size_t slotIndex) const {
		if (slotIndex >= weapons_.size()) {
			return nullptr;
		}

		return weapons_[slotIndex].get();
	}

	void Inventory::RemoveWeapon(int slotIndex) {
		if (slotIndex < 0) {
			return;
		}

		const std::size_t weaponIndex = static_cast<std::size_t>(slotIndex);
		if (weaponIndex >= weapons_.size()) {
			return;
		}

		if (weapons_[weaponIndex]) {
			weapons_[weaponIndex].reset();
			++revision_;
		}
	}

	void Inventory::DrawImGui() {

#ifdef USE_IMGUI

		if (weapons_.size() < static_cast<std::size_t>(slotCount_)) {
			weapons_.resize(slotCount_);
		}

		const int weaponCount = static_cast<int>(GetWeaponCount());

		ImGui::Begin("武器インベントリ");

		ImGui::Text("武器スロット: %d / %d", weaponCount, slotCount_);
		ImGui::Separator();

		const std::string selectedWeaponName = ProjectileTypeToString(selectedAddWeaponType_);
		if (ImGui::BeginCombo("追加する武器", selectedWeaponName.c_str())) {
			for (const Projectile::Type weaponType : Projectile::kPlayableWeaponTypes) {
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

		const bool canAddWeapon = HasEmptySlot() && !HasWeapon(selectedAddWeaponType_);
		if (!canAddWeapon) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("武器を追加") && !AddWeapon(selectedAddWeaponType_)) {
			Logger::Output("[Debug] ImGuiからの武器追加は拒否されました。", Logger::Level::Debug);
		}
		if (!canAddWeapon) {
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::TextDisabled(HasWeapon(selectedAddWeaponType_) ? "装備済みです" : "空きスロットがありません");
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
