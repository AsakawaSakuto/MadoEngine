#pragma once
#include "BaseWeapon.h"
#include <cstddef>
#include <cstdint>
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
		/// @return 武器を追加できた場合はtrueを返します。
		bool AddWeapon(Projectile::Type type);

		/// @brief 指定した種類の武器を所持しているか確認します。
		/// @param type 確認する武器種類です。
		/// @return 所持している場合はtrueを返します。
		bool HasWeapon(Projectile::Type type) const;

		/// @brief 武器スロットに空きがあるか確認します。
		/// @return 空きがある場合はtrueを返します。
		bool HasEmptySlot() const;

		/// @brief 現在装備している武器数を取得します。
		/// @return 現在装備している武器数です。
		std::size_t GetWeaponCount() const;

		/// @brief 指定した種類の武器を取得します。
		/// @param type 取得する武器種類です。
		/// @return 所持している武器へのポインターです。未所持の場合はnullptrを返します。
		BaseWeapon* GetWeapon(Projectile::Type type);

		/// @brief 指定した種類の武器を取得します。
		/// @param type 取得する武器種類です。
		/// @return 所持している武器へのconstポインターです。未所持の場合はnullptrを返します。
		const BaseWeapon* GetWeapon(Projectile::Type type) const;

		/// @brief 武器構成の変更番号を取得します。
		/// @return 武器の追加または削除ごとに増える変更番号です。
		std::uint64_t GetRevision() const { return revision_; }

		/// @brief 指定したスロットの武器を削除します。
		/// @param slotIndex 削除する武器スロットの番号です。
		void RemoveWeapon(int slotIndex);

		/// @brief 武器インベントリ編集用のImGuiを描画します。
		void DrawImGui();
	private:

		int slotCount_ = 4; // 武器スロットの数
		Projectile::Type selectedAddWeaponType_ = Projectile::Type::Pistol;
		std::vector<std::unique_ptr<BaseWeapon>> weapons_;
		std::uint64_t revision_ = 0;
	};
}
