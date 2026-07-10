#include "ProjectileManager.h"
#include <algorithm>

namespace Projectile {

	Manager& Manager::GetInstance() {
		static Manager instance;
		return instance;
	}

	void Manager::Update(float deltaTime) {
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

	void Manager::AddProjectile(Projectile::Type type, InitializeDesc context) {
		switch (type) {
		case Projectile::Type::Pistol: {
			auto pistol = std::make_unique<Pistol>();
			pistol->Initialize(context);
			projectiles.push_back(std::move(pistol));
			break;
		}
		case Projectile::Type::Rock: {
			auto rock = std::make_unique<Rock>();
			rock->Initialize(context);
			projectiles.push_back(std::move(rock));
			break;
		}
		case Projectile::Type::FireBall: {
			auto fireBall = std::make_unique<FireBall>();
			fireBall->Initialize(context);
			projectiles.push_back(std::move(fireBall));
			break;
		}
		case Projectile::Type::Axe: {
			auto axe = std::make_unique<Axe>();
			axe->Initialize(context);
			projectiles.push_back(std::move(axe));
			break;
		}
		default:
			break;
		}
	}
}
