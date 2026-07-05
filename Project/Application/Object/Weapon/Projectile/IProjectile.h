#pragma once

namespace Projectile {

	class IProjectile {
	public:
		virtual void Update(float deltaTime) = 0;
	};
}