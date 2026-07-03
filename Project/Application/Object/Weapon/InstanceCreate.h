#pragma once
#include "Instance/Pistol.h"
#include <memory>

namespace Weapon {

	/// @brief 武器のインスタンスを生成する関数
	/// @param type 生成する武器の種類
	/// @return 生成された武器のインスタンス。生成に失敗した場合はnullptr
	inline std::unique_ptr<IWeapon> CreateWeaponInstance(Type type) {
		switch (type) {
		case Type::Pistol:
			return std::make_unique<Pistol>();
		default:
			return nullptr;
		}
	}
}