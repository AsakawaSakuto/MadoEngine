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
}