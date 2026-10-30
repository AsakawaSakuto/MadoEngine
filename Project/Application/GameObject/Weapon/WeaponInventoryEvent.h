#pragma once
#include "Projectile/ProjectileStatus.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace Weapon {

	/// @brief 武器の装備変更種別
	enum class InventoryChangeType {
		Acquired,
		Removed,
	};

	/// @brief 武器本体の寿命から独立した装備変更通知
	struct InventoryChangeEvent {
		InventoryChangeType type = InventoryChangeType::Acquired;
		std::uint64_t weaponId = 0;
		std::size_t slotIndex = 0;
		std::string displayName;
		int level = 1;
		Projectile::Type weaponType = Projectile::Type::None;
	};

}
