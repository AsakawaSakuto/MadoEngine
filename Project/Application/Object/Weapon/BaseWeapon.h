#pragma once
#include "IWeapon.h"

namespace Weapon {
	
	class BaseWeapon : public IWeapon {
	public:
		Type GetType() const { return type_; }

		int GetUpgradeLevel() const { return upgradeLevel; }

		int GetKillCount() const { return killCount; }

	private:
		// 武器のステータス
		DefaultStatus defaultStatus_;
		SpecialStatus specialStatus_;

		// 武器の種類
		Type type_ = Type::None;

		int upgradeLevel = 0; // 武器のアップグレードレベル
		int killCount = 0;    // 武器のキル数
		int slotIndex = -1;   // 武器のスロットインデックス
	};
}