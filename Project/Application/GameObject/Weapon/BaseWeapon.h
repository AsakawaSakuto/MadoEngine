#pragma once
#include "UtilityHeaders.h"
#include "Projectile/ProjectileManager.h"
#include "Projectile/ProjectileStatus.h"
#include <string>
#include <vector>

namespace Weapon {
	
	class BaseWeapon {
	public:
		/// @brief 武器を初期化
		/// @param type 初期化する武器種類
		/// @param slotIndex 武器を格納するスロット番号
		/// @return 初期化に成功した場合はtrueを返す
		bool Initialize(Projectile::Type type, int slotIndex);

		void Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		void CreateProjectile(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		Projectile::Type GetProjectileType() const { return type_; }

		int GetUpgradeLevel() const { return upgradeLevel_; }

		int GetKillCount() const { return killCount_; }

		/// @brief 現在の武器ステータスを取得
		/// @return 現在の武器ステータスへのconst参照
		const UpgradeStatus& GetUpgradeStatus() const { return status_; }

		/// @brief 抽選可能な強化ステータス一覧を取得
		/// @return 有効かつ有限な強化ステータス一覧
		std::vector<UpgradeStatType> GetSelectableUpgradeStatTypes() const;

		/// @brief 指定した強化ステータスの加算値を計算
		/// @param statType 強化対象ステータス
		/// @param rarity 強化レアリティ
		/// @param outAmount 計算した加算値の出力先
		/// @return 計算に成功した場合はtrueを返す
		bool CalculateUpgradeAmount(UpgradeStatType statType, Rarity rarity, float& outAmount) const;

		/// @brief 指定した強化を武器へ適用します。
		/// @param statType 強化対象ステータス
		/// @param rarity 強化レアリティ
		/// @param expectedAmount 選択肢へ表示した適用予定の加算値
		/// @param outAppliedAmount 実際に適用した加算値の出力先
		/// @return 強化の適用に成功した場合はtrueを返す
		bool ApplyUpgrade(UpgradeStatType statType, Rarity rarity, float expectedAmount, float& outAppliedAmount);

	private:
		
		// 武器のステータス
		UpgradeStatus status_;
		
		// 武器の種類
		Projectile::Type type_ = Projectile::Type::None;

		int killCount_ = 0;        // 武器の総キル数
		float damageCount_ = 0.0f; // 武器の総ダメージ量

		int   shotNowCount_ = 0;  // 武器の現在射撃数
		int upgradeLevel_ = 0;    // 武器のアップグレードレベル
		int slotIndex_ = -1;      // 武器のスロットインデックス
		int projectileCount_ = 0; // 武器の発射数

		const float shotIntervalTime_ = 0.25f; // 武器のデフォルト射撃間隔

		GameTimer intervalTimer_;
		GameTimer cooldownTimer_;

		std::string weaponName_;
	};
}
