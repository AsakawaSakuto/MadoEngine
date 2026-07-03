#pragma once
#include "../IWeapon.h"

namespace Weapon {
	class Pistol : public IWeapon {
	public:

		void Initialize() override;

		void Update(float deltaTime) override;
	};
}