#pragma once
#include "Entity/Pistol.h"
#include "Entity/Rock.h"
#include <memory>
#include <vector>

namespace Projectile {

	class Manager {
	public:

		static Manager& GetInstance();

		void Update(float deltaTime);

		void AddProjectile(Projectile::Type type, InitializeDesc context);

	private:
		std::vector<std::unique_ptr<IProjectile>> projectiles;
	};
}
