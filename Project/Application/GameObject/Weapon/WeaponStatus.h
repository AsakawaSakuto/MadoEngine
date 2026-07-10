#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
namespace Weapon {

	/// @brief 武器のアップグレード値を管理する構造体
	struct UpgradeValue {
		float value = 0.0f; // 加算される値
		int maxLevel = 0;   // 最大レベル、0なら強化の選択肢に出現しない
	};

	/// @brief 武器のアップグレードしたステータスを管理する構造体
	struct UpgradeStatus {
		UpgradeValue damage           = { 1.0f, 99 }; // 武器のダメージ量
		UpgradeValue shotMaxCount     = { 1.0f, 99 }; // 武器の最大射撃数
		UpgradeValue shotCooldown     = { 1.0f, 99 }; // 武器の射撃クールダウン
		UpgradeValue criticalChance   = { 1.0f, 99 }; // 武器のクリティカル率
		UpgradeValue criticalDamage   = { 1.0f, 99 }; // 武器のクリティカルダメージ倍率
		UpgradeValue size             = { 1.0f, 99 }; // 武器のサイズ
		UpgradeValue bounceCount      = { 1.0f, 99 }; // 武器の跳弾回数
		UpgradeValue penetrationCount = { 1.0f, 99 }; // 武器の貫通回数
		UpgradeValue knockbackPower   = { 1.0f, 99 }; // 武器のノックバック力
		UpgradeValue lifeTime         = { 1.0f, 99 }; // 武器の弾の寿命
		UpgradeValue speed            = { 1.0f, 99 }; // 武器の弾の速度
	};

	/// @brief アップグレード値をJsonへ変換します。
	/// @param value 変換するアップグレード値です。
	/// @return 変換後のJsonです。
	inline nlohmann::json UpgradeValueToJson(const UpgradeValue& value) {
		return {
			{ "value", value.value },
			{ "maxLevel", value.maxLevel },
		};
	}

	/// @brief Jsonからアップグレード値を読み込みます。
	/// @param json 読み込み元のJsonです。
	/// @param value 読み込み先のアップグレード値です。
	inline void UpgradeValueFromJson(const nlohmann::json& json, UpgradeValue& value) {
		if (!json.is_object()) {
			return;
		}

		value.value = json.value("value", value.value);
		value.maxLevel = json.value("maxLevel", value.maxLevel);
	}

	/// @brief 武器の初期ステータスをJsonへ変換します。
	/// @param status 変換する初期ステータスです。
	/// @return 変換後のJsonです。
	inline nlohmann::json UpgradeStatusToJson(const UpgradeStatus& status) {
		return {
			{ "damage", UpgradeValueToJson(status.damage) },
			{ "shotMaxCount", UpgradeValueToJson(status.shotMaxCount) },
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

	/// @brief Jsonから武器の初期ステータスを読み込みます。
	/// @param json 読み込み元のJsonです。
	/// @param status 読み込み先の初期ステータスです。
	inline void UpgradeStatusFromJson(const nlohmann::json& json, UpgradeStatus& status) {
		if (!json.is_object()) {
			return;
		}

		UpgradeValueFromJson(json.value("damage", nlohmann::json::object()), status.damage);
		UpgradeValueFromJson(json.value("shotMaxCount", nlohmann::json::object()), status.shotMaxCount);
		UpgradeValueFromJson(json.value("shotCooldown", nlohmann::json::object()), status.shotCooldown);
		UpgradeValueFromJson(json.value("criticalChance", nlohmann::json::object()), status.criticalChance);
		UpgradeValueFromJson(json.value("criticalDamage", nlohmann::json::object()), status.criticalDamage);
		UpgradeValueFromJson(json.value("size", nlohmann::json::object()), status.size);
		UpgradeValueFromJson(json.value("bounceCount", nlohmann::json::object()), status.bounceCount);
		UpgradeValueFromJson(json.value("penetrationCount", nlohmann::json::object()), status.penetrationCount);
		UpgradeValueFromJson(json.value("knockbackPower", nlohmann::json::object()), status.knockbackPower);
		UpgradeValueFromJson(json.value("lifeTime", nlohmann::json::object()), status.lifeTime);
		UpgradeValueFromJson(json.value("speed", nlohmann::json::object()), status.speed);
	}
}
