#pragma once
#include <string>
#include "../WeaponStatus.h"
#include "MathHeaders.h"

namespace Projectile {
	
	struct  InitializeDesc {
		std::string projectileName;
		int projectileCount;
		Vector3 ownerPosition;
		Vector3 targetPosition;
	};


	/// @brief 武器の種類を表す列挙型
	enum class Type {
		None,
		Axe,
		FireBall,
		Pistol,
		Rock,
	};

	inline std::string ProjectileTypeToString(Type type) {
		switch (type) {
		case Type::None:     return "None";
		case Type::Axe:      return "Axe";
		case Type::FireBall: return "FireBall";
		case Type::Pistol:   return "Pistol";
		case Type::Rock:     return "Rock";
		default:             return "Unknown";
		}
	}
}