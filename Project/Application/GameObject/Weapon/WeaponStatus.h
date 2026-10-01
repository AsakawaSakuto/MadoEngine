#pragma once
#include "../Rarity.h"
#include <array>
#include <cmath>
#include <optional>
#include <string>

namespace Weapon {
	/// @brief 武器の強化対象ステータスを表す列挙型
	enum class UpgradeStatType {
		None,

		Damage,
		ShotMaxCount,
		ShotIntervalTime,
		ShotCooldown,
		CriticalChance,
		CriticalDamage,
		Size,
		BounceCount,
		PenetrationCount,
		KnockbackPower,
		LifeTime,
		Speed,
		BurnApplyChance,
		BurnDamage,
		BurnDuration,
		PoisonApplyChance,
		PoisonDamage,
		PoisonDuration,
		FrozenApplyChance,
		FrozenSlowRate,
		FrozenDuration,
	};

	// 抽選対象の強化ステータスを一か所で管理
	inline constexpr std::array<UpgradeStatType, 21> kUpgradeStatTypes = {
		UpgradeStatType::Damage,
		UpgradeStatType::ShotMaxCount,
		UpgradeStatType::ShotIntervalTime,
		UpgradeStatType::ShotCooldown,
		UpgradeStatType::CriticalChance,
		UpgradeStatType::CriticalDamage,
		UpgradeStatType::Size,
		UpgradeStatType::BounceCount,
		UpgradeStatType::PenetrationCount,
		UpgradeStatType::KnockbackPower,
		UpgradeStatType::LifeTime,
		UpgradeStatType::Speed,
		UpgradeStatType::BurnApplyChance,
		UpgradeStatType::BurnDamage,
		UpgradeStatType::BurnDuration,
		UpgradeStatType::PoisonApplyChance,
		UpgradeStatType::PoisonDamage,
		UpgradeStatType::PoisonDuration,
		UpgradeStatType::FrozenApplyChance,
		UpgradeStatType::FrozenSlowRate,
		UpgradeStatType::FrozenDuration,
	};

	/// @brief 武器の現在値と強化設定を管理する構造体
	struct UpgradeValue {
		float value = 0.0f;          // 現在値
		float fixedAddValue = 0.0f;  // 強化時の固定加算値
		float rarityAddValue = 0.0f; // レアリティ値ごとの上昇幅
		bool isSelected = false;     // 選択肢に出るかどうか
	};

	/// @brief 継続ダメージ型状態異常の強化設定
	struct DamageStatusEffectUpgradeStatus {
		UpgradeValue applyChance = { 0.0f, 5.0f, 1.0f, true };
		UpgradeValue damagePerTick = { 1.0f, 1.0f, 0.25f, true };
		UpgradeValue duration = { 1.0f, 1.0f, 0.25f, true };
	};

	/// @brief 凍結状態異常の強化設定
	struct FrozenStatusEffectUpgradeStatus {
		UpgradeValue applyChance = { 0.0f, 5.0f, 1.0f, true };
		UpgradeValue slowRate = { 50.0f, 5.0f, 1.0f, true };
		UpgradeValue duration = { 1.0f, 1.0f, 0.25f, true };
	};

	/// @brief 武器が付与できる状態異常の強化設定
	struct StatusEffectUpgradeStatus {
		std::optional<DamageStatusEffectUpgradeStatus> burn;
		std::optional<DamageStatusEffectUpgradeStatus> poison;
		std::optional<FrozenStatusEffectUpgradeStatus> frozen;
	};

	/// @brief 継続ダメージ型状態異常の実行時設定
	struct DamageStatusEffectStatus {
		float applyChance = 0.0f;
		float damagePerTick = 1.0f;
		float duration = 1.0f;
	};

	/// @brief 凍結状態異常の実行時設定
	struct FrozenStatusEffectStatus {
		float applyChance = 0.0f;
		float moveSpeedMultiplier = 0.5f;
		float duration = 1.0f;
	};

	/// @brief 武器が付与できる状態異常の実行時設定
	struct StatusEffectStatus {
		std::optional<DamageStatusEffectStatus> burn;
		std::optional<DamageStatusEffectStatus> poison;
		std::optional<FrozenStatusEffectStatus> frozen;
	};

	/// @brief 武器の強化ステータスを管理する構造体
	struct UpgradeStatus {
		UpgradeValue damage           = { 1.0f, 1.0f, 0.1f, true }; // 武器のダメージ量
		UpgradeValue shotMaxCount     = { 1.0f, 1.0f, 0.1f, true }; // 武器の最大射撃数
		UpgradeValue shotIntervalTime = { 0.25f, 0.0f, 0.0f, false }; // 武器の射撃間隔
		UpgradeValue shotCooldown     = { 1.0f, 1.0f, 0.1f, true }; // 武器の射撃クールダウンと強化時の短縮率
		UpgradeValue criticalChance   = { 1.0f, 1.0f, 0.1f, true }; // 武器のクリティカル率
		UpgradeValue criticalDamage   = { 1.0f, 1.0f, 0.1f, true }; // 武器のクリティカルダメージ倍率
		UpgradeValue size             = { 1.0f, 1.0f, 0.1f, true }; // 武器のサイズ
		UpgradeValue bounceCount      = { 1.0f, 1.0f, 0.1f, true }; // 武器の跳弾回数
		UpgradeValue penetrationCount = { 1.0f, 1.0f, 0.1f, true }; // 武器の貫通回数
		UpgradeValue knockbackPower   = { 1.0f, 1.0f, 0.1f, true }; // 武器のノックバック力
		UpgradeValue lifeTime         = { 1.0f, 1.0f, 0.1f, true }; // 武器の弾の寿命
		UpgradeValue speed            = { 1.0f, 1.0f, 0.1f, true }; // 武器の弾の速度
	};

	/// @brief 強化ステータスの日本語表示名を取得
	/// @param type 表示名を取得する強化ステータス
	/// @return 強化ステータスの日本語表示名
	inline const char* UpgradeStatTypeToDisplayName(UpgradeStatType type) {
		switch (type) {
		case UpgradeStatType::Damage:           return "ダメージ量";
		case UpgradeStatType::ShotMaxCount:     return "最大射撃数";
		case UpgradeStatType::ShotIntervalTime: return "射撃間隔";
		case UpgradeStatType::ShotCooldown:     return "射撃クールダウン";
		case UpgradeStatType::CriticalChance:   return "クリティカル率";
		case UpgradeStatType::CriticalDamage:   return "クリティカル倍率";
		case UpgradeStatType::Size:             return "サイズ";
		case UpgradeStatType::BounceCount:      return "跳弾回数";
		case UpgradeStatType::PenetrationCount: return "貫通回数";
		case UpgradeStatType::KnockbackPower:   return "ノックバック力";
		case UpgradeStatType::LifeTime:         return "弾の寿命";
		case UpgradeStatType::Speed:            return "弾の速度";
		case UpgradeStatType::BurnApplyChance:  return "火傷の付与率";
		case UpgradeStatType::BurnDamage:       return "火傷のダメージ";
		case UpgradeStatType::BurnDuration:     return "火傷の持続時間";
		case UpgradeStatType::PoisonApplyChance: return "毒の付与率";
		case UpgradeStatType::PoisonDamage:     return "毒のダメージ";
		case UpgradeStatType::PoisonDuration:   return "毒の持続時間";
		case UpgradeStatType::FrozenApplyChance: return "凍結の付与率";
		case UpgradeStatType::FrozenSlowRate:   return "凍結の減速率";
		case UpgradeStatType::FrozenDuration:   return "凍結の持続時間";
		default:                                return "なし";
		}
	}

	/// @brief 指定した強化ステータスの設定を取得
	/// @param status 参照する武器ステータス
	/// @param type 取得する強化ステータス
	/// @return 設定が存在する場合はconstポインターを、存在しない場合はnullptr
	inline const UpgradeValue* FindUpgradeValue(const UpgradeStatus& status, UpgradeStatType type) {
		switch (type) {
		case UpgradeStatType::Damage:           return &status.damage;
		case UpgradeStatType::ShotMaxCount:     return &status.shotMaxCount;
		case UpgradeStatType::ShotIntervalTime: return &status.shotIntervalTime;
		case UpgradeStatType::ShotCooldown:     return &status.shotCooldown;
		case UpgradeStatType::CriticalChance:   return &status.criticalChance;
		case UpgradeStatType::CriticalDamage:   return &status.criticalDamage;
		case UpgradeStatType::Size:             return &status.size;
		case UpgradeStatType::BounceCount:      return &status.bounceCount;
		case UpgradeStatType::PenetrationCount: return &status.penetrationCount;
		case UpgradeStatType::KnockbackPower:   return &status.knockbackPower;
		case UpgradeStatType::LifeTime:         return &status.lifeTime;
		case UpgradeStatType::Speed:            return &status.speed;
		default:                                return nullptr;
		}
	}

	/// @brief 指定した強化ステータスの変更可能な設定を取得
	/// @param status 参照する武器ステータス
	/// @param type 取得する強化ステータス
	/// @return 設定が存在する場合はポインターを、存在しない場合はnullptr
	inline UpgradeValue* FindUpgradeValue(UpgradeStatus& status, UpgradeStatType type) {
		return const_cast<UpgradeValue*>(
			FindUpgradeValue(static_cast<const UpgradeStatus&>(status), type));
	}

	/// @brief 指定した状態異常強化ステータスの設定を取得
	/// @param status 参照する状態異常強化設定
	/// @param type 取得する強化ステータス
	/// @return 設定が存在する場合はconstポインターを、存在しない場合はnullptr
	inline const UpgradeValue* FindUpgradeValue(
		const StatusEffectUpgradeStatus& status,
		UpgradeStatType type) {
		switch (type) {
		case UpgradeStatType::BurnApplyChance:
			return status.burn ? &status.burn->applyChance : nullptr;
		case UpgradeStatType::BurnDamage:
			return status.burn ? &status.burn->damagePerTick : nullptr;
		case UpgradeStatType::BurnDuration:
			return status.burn ? &status.burn->duration : nullptr;
		case UpgradeStatType::PoisonApplyChance:
			return status.poison ? &status.poison->applyChance : nullptr;
		case UpgradeStatType::PoisonDamage:
			return status.poison ? &status.poison->damagePerTick : nullptr;
		case UpgradeStatType::PoisonDuration:
			return status.poison ? &status.poison->duration : nullptr;
		case UpgradeStatType::FrozenApplyChance:
			return status.frozen ? &status.frozen->applyChance : nullptr;
		case UpgradeStatType::FrozenSlowRate:
			return status.frozen ? &status.frozen->slowRate : nullptr;
		case UpgradeStatType::FrozenDuration:
			return status.frozen ? &status.frozen->duration : nullptr;
		default:
			return nullptr;
		}
	}

	/// @brief 指定した状態異常強化ステータスの変更可能な設定を取得
	/// @param status 参照する状態異常強化設定
	/// @param type 取得する強化ステータス
	/// @return 設定が存在する場合はポインターを、存在しない場合はnullptr
	inline UpgradeValue* FindUpgradeValue(
		StatusEffectUpgradeStatus& status,
		UpgradeStatType type) {
		return const_cast<UpgradeValue*>(
			FindUpgradeValue(static_cast<const StatusEffectUpgradeStatus&>(status), type));
	}

	/// @brief レアリティが武器強化の抽選対象か確認
	/// @param rarity 確認するレアリティ
	/// @return UncommonからLegendaryの場合はtrue
	inline bool IsWeaponUpgradeRarity(Rarity rarity) {
		const int rarityValue = static_cast<int>(rarity);
		return rarityValue >= static_cast<int>(Rarity::Uncommon) &&
			rarityValue <= static_cast<int>(Rarity::Legendary);
	}

	/// @brief アップグレード値の有限性を検証
	/// @param value 検証対象のアップグレード値
	/// @return すべての数値が有限の場合はtrue
	inline bool IsFiniteUpgradeValue(const UpgradeValue& value) {
		return std::isfinite(value.value) &&
			std::isfinite(value.fixedAddValue) &&
			std::isfinite(value.rarityAddValue);
	}

	/// @brief 継続ダメージ型状態異常の強化設定を検証
	/// @param status 検証対象の状態異常強化設定
	/// @return 有効な設定の場合はtrue
	inline bool IsValidDamageStatusEffectUpgradeStatus(
		const DamageStatusEffectUpgradeStatus& status) {
		return IsFiniteUpgradeValue(status.applyChance) &&
			status.applyChance.value >= 0.0f && status.applyChance.value <= 100.0f &&
			IsFiniteUpgradeValue(status.damagePerTick) && status.damagePerTick.value > 0.0f &&
			IsFiniteUpgradeValue(status.duration) && status.duration.value > 0.0f;
	}

	/// @brief 凍結状態異常の強化設定を検証
	/// @param status 検証対象の状態異常強化設定
	/// @return 有効な設定の場合はtrue
	inline bool IsValidFrozenStatusEffectUpgradeStatus(
		const FrozenStatusEffectUpgradeStatus& status) {
		return IsFiniteUpgradeValue(status.applyChance) &&
			status.applyChance.value >= 0.0f && status.applyChance.value <= 100.0f &&
			IsFiniteUpgradeValue(status.slowRate) &&
			status.slowRate.value > 0.0f && status.slowRate.value <= 100.0f &&
			IsFiniteUpgradeValue(status.duration) && status.duration.value > 0.0f;
	}

	/// @brief 武器に状態異常強化設定が存在するか確認
	/// @param status 確認対象の状態異常強化設定
	/// @return 一種類以上の設定が存在する場合はtrue
	inline bool HasStatusEffect(const StatusEffectUpgradeStatus& status) {
		return status.burn.has_value() || status.poison.has_value() || status.frozen.has_value();
	}

	/// @brief 武器の状態異常強化設定を検証
	/// @param status 検証対象の状態異常強化設定
	/// @return すべての設定が有効な場合はtrue
	inline bool IsValidStatusEffectUpgradeStatus(const StatusEffectUpgradeStatus& status) {
		return (!status.burn || IsValidDamageStatusEffectUpgradeStatus(*status.burn)) &&
			(!status.poison || IsValidDamageStatusEffectUpgradeStatus(*status.poison)) &&
			(!status.frozen || IsValidFrozenStatusEffectUpgradeStatus(*status.frozen));
	}

	/// @brief 状態異常強化設定から実行時設定を生成
	/// @param status 変換する状態異常強化設定
	/// @return 現在値だけを格納した実行時設定
	inline StatusEffectStatus ResolveStatusEffectStatus(const StatusEffectUpgradeStatus& status) {
		constexpr float kPercentageScale = 0.01f;
		StatusEffectStatus resolvedStatus;
		if (status.burn) {
			resolvedStatus.burn = DamageStatusEffectStatus{
				status.burn->applyChance.value,
				status.burn->damagePerTick.value,
				status.burn->duration.value,
			};
		}
		if (status.poison) {
			resolvedStatus.poison = DamageStatusEffectStatus{
				status.poison->applyChance.value,
				status.poison->damagePerTick.value,
				status.poison->duration.value,
			};
		}
		if (status.frozen) {
			resolvedStatus.frozen = FrozenStatusEffectStatus{
				status.frozen->applyChance.value,
				1.0f - status.frozen->slowRate.value * kPercentageScale,
				status.frozen->duration.value,
			};
		}

		return resolvedStatus;
	}

} // namespace Weapon
