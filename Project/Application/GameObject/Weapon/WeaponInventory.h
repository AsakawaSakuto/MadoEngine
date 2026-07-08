#pragma once
#include "BaseWeapon.h"
#include <memory>
#include <vector>

namespace Weapon {

	/// @brief 武器のインベントリを管理するクラス
	class Inventory {
	public:
		void Initialize(Projectile::Type type);

		void Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		/// @brief 指定した種類の武器を一番左の空きスロットへ追加する
		/// @param type 追加する武器の種類
		void AddWeapon(Projectile::Type type);

		/// @brief 指定したスロットの武器を削除します。
		/// @param slotIndex 削除する武器スロットの番号です。
		void RemoveWeapon(int slotIndex);

		/// @brief 武器インベントリ編集用のImGuiを描画します。
		void DrawImGui();
	private:

		int slotCount_ = 4; // 武器スロットの数
		Projectile::Type selectedAddWeaponType_ = Projectile::Type::Pistol;
		std::vector<std::unique_ptr<BaseWeapon>> weapons_;
	};
}
