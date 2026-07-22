#pragma once
#include "../Rarity.h"
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
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
	};

	// 抽選対象の強化ステータスを一か所で管理
	inline constexpr std::array<UpgradeStatType, 12> kUpgradeStatTypes = {
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
	};

	/// @brief 武器の現在値と強化設定を管理する構造体
	struct UpgradeValue {
		float value = 0.0f;          // 現在値
		float fixedAddValue = 0.0f;  // 強化時の固定加算値
		float rarityAddValue = 0.0f; // レアリティ値ごとの上昇幅
		bool isSelected = false;     // 選択肢に出るかどうか
	};

	/// @brief 武器の強化ステータスを管理する構造体
	struct UpgradeStatus {
		UpgradeValue damage           = { 1.0f, 1.0f, 0.1f, true }; // 武器のダメージ量
		UpgradeValue shotMaxCount     = { 1.0f, 1.0f, 0.1f, true }; // 武器の最大射撃数
		UpgradeValue shotIntervalTime = { 0.25f, 0.0f, 0.0f, false }; // 武器の射撃間隔
		UpgradeValue shotCooldown     = { 1.0f, 1.0f,-0.1f, true }; // 武器の射撃クールダウン
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
		default:                                return "なし";
		}
	}

	/// @brief 指定した強化ステータスの設定を取得
	/// @param status 参照する武器ステータス
	/// @param type 取得する強化ステータス
	/// @return 設定が存在する場合はconstポインターを、存在しない場合はnullptrを返す
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

	/// @brief レアリティが武器強化の抽選対象か確認
	/// @param rarity 確認するレアリティ
	/// @return UncommonからLegendaryの場合はtrueを返す
	inline bool IsWeaponUpgradeRarity(Rarity rarity) {
		const int rarityValue = static_cast<int>(rarity);
		return rarityValue >= static_cast<int>(Rarity::Uncommon) &&
			rarityValue <= static_cast<int>(Rarity::Legendary);
	}

	/// @brief アップグレード値をJsonへ変換
	/// @param value 変換するアップグレード値
	/// @return 変換後のJson
	inline nlohmann::json UpgradeValueToJson(const UpgradeValue& value) {
		return {
			{ "value", value.value },
			{ "fixedAddValue", value.fixedAddValue },
			{ "rarityAddValue", value.rarityAddValue },
			{ "isSelected", value.isSelected },
		};
	}

	/// @brief Jsonからアップグレード値を読み込み
	/// @param json 読み込み元のJson
	/// @param value 読み込み先のアップグレード値
	/// @return 有効な値を読み込めた場合はtrueを返す
	inline bool UpgradeValueFromJson(const nlohmann::json& json, UpgradeValue& value) {
		if (!json.is_object()) {
			return false;
		}

		UpgradeValue parsedValue = value;
		auto readFiniteFloat = [&json](const char* key, float& destination) {
			if (!json.contains(key)) {
				return true;
			}

			const nlohmann::json& source = json.at(key);
			if (!source.is_number()) {
				return false;
			}

			const float parsed = source.get<float>();
			if (!std::isfinite(parsed)) {
				return false;
			}

			destination = parsed;
			return true;
		};

		if (!readFiniteFloat("value", parsedValue.value) ||
			!readFiniteFloat("fixedAddValue", parsedValue.fixedAddValue) ||
			!readFiniteFloat("rarityAddValue", parsedValue.rarityAddValue)) {
			return false;
		}

		if (json.contains("isSelected")) {
			if (!json.at("isSelected").is_boolean()) {
				return false;
			}
			parsedValue.isSelected = json.at("isSelected").get<bool>();
		}

		value = parsedValue;
		return true;
	}

	/// @brief 武器の初期ステータスをJsonへ変換
	/// @param status 変換する初期ステータス
	/// @return 変換後のJson
	inline nlohmann::json UpgradeStatusToJson(const UpgradeStatus& status) {
		return {
			{ "damage", UpgradeValueToJson(status.damage) },
			{ "shotMaxCount", UpgradeValueToJson(status.shotMaxCount) },
			{ "shotIntervalTime", UpgradeValueToJson(status.shotIntervalTime) },
			{ "shotCooldown", UpgradeValueToJson(status.shotCooldown) },
			{ "criticalChance", UpgradeValueToJson(status.criticalChance) },
			{ "criticalDamage", UpgradeValueToJson(status.criticalDamage) },
			{ "size", UpgradeValueToJson(status.size) },
			{ "bounceCount", UpgradeValueToJson(status.bounceCount) },
			{ "penetrationCount", UpgradeValueToJson(status.penetrationCount) },
			{ "knockbackPower", UpgradeValueToJson(status.knockbackPower) },
			{ "lifeTime", UpgradeValueToJson(status.lifeTime) },
			{ "speed", UpgradeValueToJson(status.speed) },
		};
	}

	/// @brief Jsonから武器の初期ステータスを読み込み
	/// @param json 読み込み元のJson
	/// @param status 読み込み先の初期ステータス
	/// @return 有効なステータスを読み込めた場合はtrueを返す
	inline bool UpgradeStatusFromJson(const nlohmann::json& json, UpgradeStatus& status) {
		if (!json.is_object()) {
			return false;
		}

		UpgradeStatus parsedStatus = status;
		auto readValue = [&json](const char* key, UpgradeValue& destination) {
			if (!json.contains(key)) {
				return true;
			}
			return UpgradeValueFromJson(json.at(key), destination);
		};

		if (!readValue("damage", parsedStatus.damage) ||
			!readValue("shotMaxCount", parsedStatus.shotMaxCount) ||
			!readValue("shotIntervalTime", parsedStatus.shotIntervalTime) ||
			!readValue("shotCooldown", parsedStatus.shotCooldown) ||
			!readValue("criticalChance", parsedStatus.criticalChance) ||
			!readValue("criticalDamage", parsedStatus.criticalDamage) ||
			!readValue("size", parsedStatus.size) ||
			!readValue("bounceCount", parsedStatus.bounceCount) ||
			!readValue("penetrationCount", parsedStatus.penetrationCount) ||
			!readValue("knockbackPower", parsedStatus.knockbackPower) ||
			!readValue("lifeTime", parsedStatus.lifeTime) ||
			!readValue("speed", parsedStatus.speed)) {
			return false;
		}

		status = parsedStatus;
		return true;
	}
}
