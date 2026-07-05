#pragma once
#include "BaseWeapon.h"

namespace Weapon {

	/// @brief 武器のインベントリを管理するクラス
	class Inventory {
	public:
		void Initialize();

		void Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		/// @brief 武器を追加する
		void AddWeapon() {};
	private:

		int slotCount_ = 4; // 武器スロットの数
		std::vector<std::unique_ptr<BaseWeapon>> weapons_;
	};
}