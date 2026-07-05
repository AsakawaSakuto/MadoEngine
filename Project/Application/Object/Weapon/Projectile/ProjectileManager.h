#pragma once
#include "Entity/Pistol.h"
#include "Entity/Rock.h"

namespace Projectile {

	class ProjectileManager {
	public:

		static ProjectileManager& GetInstance();

		void Update(float deltaTime);

		void AddProjectile(Weapon::Type type, const std::string& entityName, int projectileCount);

	private:
		std::vector<std::unique_ptr<IProjectile>> projectiles;
	};
}