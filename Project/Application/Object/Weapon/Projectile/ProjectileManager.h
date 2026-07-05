#pragma once
#include "Entity/Pistol.h"
#include "Entity/Rock.h"
#include <memory>
#include <vector>

namespace Projectile {

	class ProjectileManager {
	public:

		static ProjectileManager& GetInstance();

		void Update(float deltaTime);

		void AddProjectile(Weapon::Type type, InitializeContext context);

	private:
		std::vector<std::unique_ptr<IProjectile>> projectiles;
	};
}
