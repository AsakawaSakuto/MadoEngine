#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "../WeaponStatus.h"
#include <vector>
#include <memory>
#include <string>

namespace Projectile {

	class IProjectile {
	public:
		virtual void Initialize(const std::string& entityName, int projectileCount) = 0;

		virtual void Update(float deltaTime) = 0;

	protected:

		Transform3D transform_;
		ColliderShape hitbox_;
	};
}