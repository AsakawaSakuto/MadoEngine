#include "WeaponStatusJson.h"
#include <cmath>
#include <nlohmann/json.hpp>

namespace Weapon {

	nlohmann::json UpgradeValueToJson(const UpgradeValue& value) {
		return {
			{ "value", value.value },
			{ "fixedAddValue", value.fixedAddValue },
			{ "rarityAddValue", value.rarityAddValue },
			{ "isSelected", value.isSelected },
		};
	}

	bool UpgradeValueFromJson(const nlohmann::json& json, UpgradeValue& value) {
		if (!json.is_object()) {
			return false;
		}

		UpgradeValue parsedValue = value;

		// 未指定項目は既存値を保ち、指定項目だけを有限値として検証
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

	nlohmann::json DamageStatusEffectUpgradeStatusToJson(
		const DamageStatusEffectUpgradeStatus& status) {
		return {
			{ "applyChance", UpgradeValueToJson(status.applyChance) },
			{ "damagePerTick", UpgradeValueToJson(status.damagePerTick) },
			{ "duration", UpgradeValueToJson(status.duration) },
		};
	}

	nlohmann::json FrozenStatusEffectUpgradeStatusToJson(
		const FrozenStatusEffectUpgradeStatus& status) {
		return {
			{ "applyChance", UpgradeValueToJson(status.applyChance) },
			{ "slowRate", UpgradeValueToJson(status.slowRate) },
			{ "duration", UpgradeValueToJson(status.duration) },
		};
	}

	nlohmann::json StatusEffectUpgradeStatusToJson(const StatusEffectUpgradeStatus& status) {
		nlohmann::json json = nlohmann::json::object();

		// 武器で有効化した状態異常だけを保存して未使用設定を明示的に除外
		if (status.burn) {
			json["burn"] = DamageStatusEffectUpgradeStatusToJson(*status.burn);
		}
		if (status.poison) {
			json["poison"] = DamageStatusEffectUpgradeStatusToJson(*status.poison);
		}
		if (status.frozen) {
			json["frozen"] = FrozenStatusEffectUpgradeStatusToJson(*status.frozen);
		}
		return json;
	}

	bool DamageStatusEffectUpgradeStatusFromJson(
		const nlohmann::json& json,
		DamageStatusEffectUpgradeStatus& status) {
		if (!json.is_object() ||
			!json.contains("applyChance") ||
			!json.contains("damagePerTick") ||
			!json.contains("duration")) {
			return false;
		}

		DamageStatusEffectUpgradeStatus parsedStatus;
		if (!UpgradeValueFromJson(json.at("applyChance"), parsedStatus.applyChance) ||
			!UpgradeValueFromJson(json.at("damagePerTick"), parsedStatus.damagePerTick) ||
			!UpgradeValueFromJson(json.at("duration"), parsedStatus.duration) ||
			!IsValidDamageStatusEffectUpgradeStatus(parsedStatus)) {
			return false;
		}

		status = parsedStatus;
		return true;
	}

	bool FrozenStatusEffectUpgradeStatusFromJson(
		const nlohmann::json& json,
		FrozenStatusEffectUpgradeStatus& status) {
		if (!json.is_object() ||
			!json.contains("applyChance") ||
			!json.contains("slowRate") ||
			!json.contains("duration")) {
			return false;
		}

		FrozenStatusEffectUpgradeStatus parsedStatus;
		if (!UpgradeValueFromJson(json.at("applyChance"), parsedStatus.applyChance) ||
			!UpgradeValueFromJson(json.at("slowRate"), parsedStatus.slowRate) ||
			!UpgradeValueFromJson(json.at("duration"), parsedStatus.duration) ||
			!IsValidFrozenStatusEffectUpgradeStatus(parsedStatus)) {
			return false;
		}

		status = parsedStatus;
		return true;
	}

	bool StatusEffectUpgradeStatusFromJson(
		const nlohmann::json& json,
		StatusEffectUpgradeStatus& status) {
		if (!json.is_object()) {
			return false;
		}

		StatusEffectUpgradeStatus parsedStatus;

		// 状態異常ごとに独立検証し、一項目でも不正なら既存設定を維持
		if (json.contains("burn")) {
			DamageStatusEffectUpgradeStatus burn;
			if (!DamageStatusEffectUpgradeStatusFromJson(json.at("burn"), burn)) {
				return false;
			}
			parsedStatus.burn = burn;
		}
		if (json.contains("poison")) {
			DamageStatusEffectUpgradeStatus poison;
			if (!DamageStatusEffectUpgradeStatusFromJson(json.at("poison"), poison)) {
				return false;
			}
			parsedStatus.poison = poison;
		}
		if (json.contains("frozen")) {
			FrozenStatusEffectUpgradeStatus frozen;
			if (!FrozenStatusEffectUpgradeStatusFromJson(json.at("frozen"), frozen)) {
				return false;
			}
			parsedStatus.frozen = frozen;
		}

		status = parsedStatus;
		return true;
	}

	nlohmann::json UpgradeStatusToJson(const UpgradeStatus& status) {
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

	bool UpgradeStatusFromJson(const nlohmann::json& json, UpgradeStatus& status) {
		if (!json.is_object()) {
			return false;
		}

		UpgradeStatus parsedStatus = status;

		// 旧形式との互換性を保つため存在する項目だけを読み込み
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

} // namespace Weapon
