#pragma once
#include "../IProjectile.h"

namespace Projectile {

	class Rock : public IProjectile {
	public:
		void Initialize(InitializeDesc context) override;

		void Update(float deltaTime) override;

	private:
	};
}
