#pragma once
#include "../IProjectile.h"

namespace Projectile {

	class Rock : public IProjectile {
	public:
		void Initialize(const std::string& entityName, int projectileCount) override;

		void Update(float deltaTime) override;

	private:
	};
}
