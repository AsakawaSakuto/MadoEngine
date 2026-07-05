#pragma once
#include "BaseWeapon.h"

namespace Weapon {

	class Inventory {
	public:
		void Initialize();

		void Update(float deltaTime);

	private:

		int slotCount_ = 4; // 武器スロットの数
		std::vector<std::unique_ptr<BaseWeapon>> weapons_;
	};
}