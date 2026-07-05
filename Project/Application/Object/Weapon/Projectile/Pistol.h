#pragma once
#include "IProjectile.h"

namespace Projectile {

	class Pistol : public IProjectile {
	public:
		void Update(float deltaTime) override;

	private:
	};
}