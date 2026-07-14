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

		/// @brief 指定した種類の武器を一番左の空きスロットへ追加
		/// @param type 追加する武器の種類
		/// @return 武器を追加できた場合はtrueを返す
		bool AddWeapon(Projectile::Type type);

		/// @brief 指定した種類の武器を所持しているか確認
		/// @param type 確認する武器種類
		/// @return 所持している場合はtrueを返す
		bool HasWeapon(Projectile::Type type) const;

		/// @brief 武器スロットに空きがあるか確認
		/// @return 空きがある場合はtrueを返す
		bool HasEmptySlot() const;

		/// @brief 現在装備している武器数を取得
		/// @return 現在装備している武器数
		std::size_t GetWeaponCount() const;

		/// @brief 指定した種類の武器を取得
		/// @param type 取得する武器種類
		/// @return 所持している武器へのポインター。未所持の場合はnullptrを返す
		BaseWeapon* GetWeapon(Projectile::Type type);

		/// @brief 指定した種類の武器を取得
		/// @param type 取得する武器種類
		/// @return 所持している武器へのconstポインター。未所持の場合はnullptrを返す
		const BaseWeapon* GetWeapon(Projectile::Type type) const;

		/// @brief 指定したスロットに装備されている武器を取得します。
		/// @param slotIndex 取得する武器スロットの番号
		/// @return 装備中の武器へのconstポインター。空きスロットまたは範囲外の場合はnullptrを返す
		const BaseWeapon* GetWeaponAtSlot(std::size_t slotIndex) const;

		/// @brief 武器構成の変更番号を取得
		/// @return 武器の追加または削除ごとに増える変更番号
		std::uint64_t GetRevision() const { return revision_; }

		/// @brief 指定したスロットの武器を削除
		/// @param slotIndex 削除する武器スロットの番号
		void RemoveWeapon(int slotIndex);

		/// @brief 武器インベントリ編集用のImGuiを描画
		void DrawImGui();
	private:

		int slotCount_ = 4; // 武器スロットの数
		Projectile::Type selectedAddWeaponType_ = Projectile::Type::Pistol;
		std::vector<std::unique_ptr<BaseWeapon>> weapons_;
		std::uint64_t revision_ = 0;
	};
}
