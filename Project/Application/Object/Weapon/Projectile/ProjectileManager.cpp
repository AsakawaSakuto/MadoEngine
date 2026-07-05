#include "ProjectileManager.h"

namespace Projectile {

	ProjectileManager& ProjectileManager::GetInstance() {
		static ProjectileManager instance;
		return instance;
	}

	void ProjectileManager::Update(float deltaTime) {
		for (auto& projectile : projectiles) {
			projectile->Update(deltaTime);
		}
	}

	void ProjectileManager::AddProjectile(Weapon::Type type) {
		switch (type) {
		case Weapon::Type::Pistol:
			projectiles.push_back(std::make_unique<Pistol>());
			break;
		default:
			break;
		}
	}
}
