#pragma once
#include "BaseWeapon.h"
#include <memory>
#include <vector>

namespace Weapon {

	/// @brief 武器のインベントリを管理するクラス
	class Inventory {
	public:
		void Initialize(Type type);

		void Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		/// @brief 指定した種類の武器を一番左の空きスロットへ追加する
		/// @param type 追加する武器の種類
		void AddWeapon(Type type);
	private:

		int slotCount_ = 4; // 武器スロットの数
		std::vector<std::unique_ptr<BaseWeapon>> weapons_;
	};
}
