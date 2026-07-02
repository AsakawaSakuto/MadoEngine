#pragma once
#include "WeaponStatus.h"

namespace Weapon {
	/// @brief 武器のインターフェースクラス
	class IWeapon {
	public:
		virtual ~IWeapon() = default;
		
	private:
		// 武器のステータス
		DefultStatus defultStatus_;
		SpecialStatus specialStatus_;

		// 武器の種類
		Type type_ = Type::None;
	};
}