#pragma once
#include "../IProjectile.h"

namespace Projectile {

	class Pistol : public IProjectile {
	public:

		void Initialize(InitializeDesc context) override;

		void Update(float deltaTime) override;

	private:

		Model* model_ = nullptr;
		Vector3 moveDirection_ = { 0.0f, 0.0f, 0.0f };
	};
}
