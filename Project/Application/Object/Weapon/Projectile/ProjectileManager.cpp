#include "ProjectileManager.h"
#include <algorithm>

namespace Projectile {

	ProjectileManager& ProjectileManager::GetInstance() {
		static ProjectileManager instance;
		return instance;
	}

	void ProjectileManager::Update(float deltaTime) {
		for (auto& projectile : projectiles) {
			projectile->Update(deltaTime);
		}

		projectiles.erase(
			std::remove_if(projectiles.begin(), projectiles.end(), [](const std::unique_ptr<IProjectile>& projectile) {
				return projectile->IsDead();
			}),
			projectiles.end()
		);
	}

	void ProjectileManager::AddProjectile(Weapon::Type type, InitializeDesc context) {
		switch (type) {
		case Weapon::Type::Pistol: {
			auto pistol = std::make_unique<Pistol>();
			pistol->Initialize(context);
			projectiles.push_back(std::move(pistol));
			break;
		}
		case Weapon::Type::Rock: {
			auto rock = std::make_unique<Rock>();
			rock->Initialize(context);
			projectiles.push_back(std::move(rock));
			break;
		}
		default:
			break;
		}
	}
}
