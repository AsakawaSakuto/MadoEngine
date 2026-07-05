#pragma once
#include "Pistol.h"
#include "../WeaponStatus.h"
#include <vector>
#include <memory>

namespace Projectile {

	class ProjectileManager {
	public:

		static ProjectileManager& GetInstance();

		void Update(float deltaTime);

		void AddProjectile(Weapon::Type type);

	private:
		std::vector<std::unique_ptr<IProjectile>> projectiles;
	};
}