#pragma once
#include "WeaponStatus.h"

namespace Weapon {

	/// @brief 武器のインターフェースクラス
	class IWeapon {
	public:

		virtual void Update(float deltaTime) = 0;
	};
}