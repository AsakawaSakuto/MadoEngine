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

		/// @brief 武器の射撃処理を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param ownerPosition 武器所有者の座標
		/// @param targetPosition 射撃対象の座標
		void Update(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);

		/// @brief 直前の更新でProjectileを射出したか確認
		/// @return Projectileを射出した場合はtrueを返す
		bool WasFiredThisFrame() const { return wasFiredThisFrame_; }

		/// @brief 連射終了後のクールダウン進捗を取得
		/// @return クールダウン開始を0.0f、終了を1.0fとした進捗。連射中は1.0f
		float GetShotCooldownProgress() const {
			return cooldownTimer_.IsActive() ? cooldownTimer_.GetProgress() : 1.0f;
		}

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
		/// @brief 射撃間隔とクールダウンに応じてProjectileを射出
		/// @param deltaTime 前フレームからの経過時間
		/// @param ownerPosition 武器所有者の座標
		/// @param targetPosition 射撃対象の座標
		void CreateProjectile(float deltaTime, const Vector3& ownerPosition, const Vector3& targetPosition);
		
		// 武器のステータス
		UpgradeStatus status_;
		
		// 武器の種類
		Projectile::Type type_ = Projectile::Type::None;

		int killCount_ = 0;        // 武器の総キル数
		float damageCount_ = 0.0f; // 武器の総ダメージ量

		int shotNowCount_ = 0;  // 武器の現在射撃数
		int upgradeLevel_ = 1;    // 武器のアップグレードレベル
		int slotIndex_ = -1;      // 武器のスロットインデックス
		int projectileCount_ = 0; // 武器の発射数
		bool wasFiredThisFrame_ = false;

		GameTimer intervalTimer_;
		GameTimer cooldownTimer_;

		std::string weaponName_;
	};
}
