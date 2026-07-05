#pragma once
#include <string>
#include "../WeaponStatus.h"
#include "MathHeaders.h"

namespace Projectile {
	
	struct  InitializeContext {
		std::string projectileName;
		int projectileCount;
		Vector3 ownerPosition;
		Vector3 targetPosition;
	};
}