#include "RelicCatalog.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <cmath>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <utility>

namespace {

/// @brief 文字列から補正対象ステータスを取得
/// @param text 変換対象文字列
/// @param outStat 変換結果の出力先
/// @return 変換に成功した場合はtrue
bool TryParseStat(const std::string& text, Relic::Stat& outStat) {
	if (text == "OutgoingDamage") {
		outStat = Relic::Stat::OutgoingDamage;
		return true;
	}
	if (text == "MaxHealth") {
		outStat = Relic::Stat::MaxHealth;
		return true;
	}
	if (text == "MovementSpeed") {
		outStat = Relic::Stat::MovementSpeed;
		return true;
	}
	if (text == "WeaponShotMaxCount") {
		outStat = Relic::Stat::WeaponShotMaxCount;
		return true;
	}
	if (text == "MoneyGain") {
		outStat = Relic::Stat::MoneyGain;
		return true;
	}
	if (text == "HealthRegenerationSpeed") {
		outStat = Relic::Stat::HealthRegenerationSpeed;
		return true;
	}
	if (text == "ExpGain") {
		outStat = Relic::Stat::ExpGain;
		return true;
	}
	if (text == "ProjectileCriticalChance") {
		outStat = Relic::Stat::ProjectileCriticalChance;
		return true;
	}
	if (text == "WeaponShotCooldown") {
		outStat = Relic::Stat::WeaponShotCooldown;
		return true;
	}
	if (text == "JumpCount") {
		outStat = Relic::Stat::JumpCount;
		return true;
	}
	if (text == "IncomingDamage") {
		outStat = Relic::Stat::IncomingDamage;
		return true;
	}
	return false;
}

/// @brief 文字列から補正演算を取得
/// @param text 変換対象文字列
/// @param outOperation 変換結果の出力先
/// @return 変換に成功した場合はtrue
bool TryParseOperation(const std::string& text, Relic::ModifierOperation& outOperation) {
	if (text == "AddFlat") {
		outOperation = Relic::ModifierOperation::AddFlat;
		return true;
	}
	if (text == "AddPercent") {
		outOperation = Relic::ModifierOperation::AddPercent;
		return true;
	}
	return false;
}

/// @brief 補正対象ステータスをJSON用文字列へ変換
/// @param stat 変換対象のステータス
/// @return JSONへ保存する文字列、無効値の場合は空文字列
const char* StatToJsonName(Relic::Stat stat) {
	switch (stat) {
	case Relic::Stat::OutgoingDamage:
		return "OutgoingDamage";
	case Relic::Stat::MaxHealth:
		return "MaxHealth";
	case Relic::Stat::MovementSpeed:
		return "MovementSpeed";
	case Relic::Stat::WeaponShotMaxCount:
		return "WeaponShotMaxCount";
	case Relic::Stat::MoneyGain:
		return "MoneyGain";
	case Relic::Stat::HealthRegenerationSpeed:
		return "HealthRegenerationSpeed";
	case Relic::Stat::ExpGain:
		return "ExpGain";
	case Relic::Stat::ProjectileCriticalChance:
		return "ProjectileCriticalChance";
	case Relic::Stat::WeaponShotCooldown:
		return "WeaponShotCooldown";
	case Relic::Stat::JumpCount:
		return "JumpCount";
	case Relic::Stat::IncomingDamage:
		return "IncomingDamage";
	default:
		return "";
	}
}

/// @brief 補正演算をJSON用文字列へ変換
/// @param operation 変換対象の補正演算
/// @return JSONへ保存する文字列、無効値の場合は空文字列
const char* OperationToJsonName(Relic::ModifierOperation operation) {
	switch (operation) {
	case Relic::ModifierOperation::AddFlat:
		return "AddFlat";
	case Relic::ModifierOperation::AddPercent:
		return "AddPercent";
	default:
		return "";
	}
}

/// @brief 一件のレリック定義が実行時に利用可能か確認
/// @param definition 検証対象のレリック定義
/// @return 全項目が有効な場合はtrue
bool IsValidDefinition(const Relic::Definition& definition) {
	if (definition.id.empty() || definition.displayName.empty() || definition.textureName.empty() ||
		definition.maxStack <= 0 ||
		GetRaritySerializedName(definition.rarity)[0] == '\0' || definition.effects.empty()) {
		return false;
	}

	for (const Relic::StatModifierEffect& effect : definition.effects) {
		if (!std::isfinite(effect.valuePerStack) ||
			StatToJsonName(effect.stat)[0] == '\0' ||
			OperationToJsonName(effect.operation)[0] == '\0') {
			return false;
		}
	}
	return true;
}

/// @brief JSONから一件のレリック定義を構築
/// @param json 定義元JSON
/// @param outDefinition 構築結果の出力先
/// @return 有効な定義を構築できた場合はtrue
bool TryParseDefinition(const nlohmann::json& json, Relic::Definition& outDefinition) {
	if (!json.is_object() ||
		!json.contains("id") ||
		!json.contains("displayName") ||
		!json.contains("description") ||
		!json.contains("textureName") ||
		!json.contains("rarity") ||
		!json.contains("maxStack") ||
		!json.contains("effects")) {
		return false;
	}

	try {
		Relic::Definition parsed;
		parsed.id = json.at("id").get<std::string>();
		parsed.displayName = json.at("displayName").get<std::string>();
		parsed.description = json.at("description").get<std::string>();
		parsed.textureName = json.at("textureName").get<std::string>();
		parsed.maxStack = json.at("maxStack").get<int>();
		if (parsed.id.empty() || parsed.displayName.empty() || parsed.textureName.empty() ||
			parsed.maxStack <= 0 ||
			!TryParseRarity(json.at("rarity").get<std::string>(), parsed.rarity) ||
			!json.at("effects").is_array() || json.at("effects").empty()) {
			return false;
		}

		for (const nlohmann::json& effectJson : json.at("effects")) {
			if (!effectJson.is_object() || effectJson.value("type", std::string{}) != "StatModifier") {
				return false;
			}

			Relic::StatModifierEffect effect;
			effect.valuePerStack = effectJson.at("valuePerStack").get<float>();
			if (!std::isfinite(effect.valuePerStack) ||
				!TryParseStat(effectJson.at("stat").get<std::string>(), effect.stat) ||
				!TryParseOperation(effectJson.at("operation").get<std::string>(), effect.operation)) {
				return false;
			}
			parsed.effects.push_back(effect);
		}

		outDefinition = std::move(parsed);
		return true;
	} catch (const nlohmann::json::exception&) {
		return false;
	}
}

}

namespace Relic {

	bool Catalog::Load(const std::string& filePath) {
		nlohmann::json root;
		if (!MadoEngine::Json::JsonFile::Load(filePath, root) ||
			!root.is_object() || !root.contains("relics") || !root.at("relics").is_array()) {
			Logger::Output("[Relic] レリック定義の読み込みに失敗: " + filePath, Logger::Level::Error);
			return false;
		}

		std::vector<Definition> parsedDefinitions;
		std::unordered_set<Id> parsedIds;
		for (const nlohmann::json& definitionJson : root.at("relics")) {
			Definition definition;
			if (!TryParseDefinition(definitionJson, definition) || !parsedIds.insert(definition.id).second) {
				Logger::Output("[Relic] 不正または重複したレリック定義を検出", Logger::Level::Error);
				return false;
			}
			parsedDefinitions.push_back(std::move(definition));
		}

		if (parsedDefinitions.empty()) {
			Logger::Output("[Relic] レリック定義が存在しないため読み込みを中止", Logger::Level::Error);
			return false;
		}

		return Replace(std::move(parsedDefinitions));
	}

	bool Catalog::Save(const std::string& filePath) const {
		nlohmann::json root;
		root["relics"] = nlohmann::json::array();
		for (const Definition& definition : definitions_) {
			nlohmann::json definitionJson;
			definitionJson["id"] = definition.id;
			definitionJson["displayName"] = definition.displayName;
			definitionJson["description"] = definition.description;
			definitionJson["textureName"] = definition.textureName;
			definitionJson["rarity"] = GetRaritySerializedName(definition.rarity);
			definitionJson["maxStack"] = definition.maxStack;
			definitionJson["effects"] = nlohmann::json::array();
			for (const StatModifierEffect& effect : definition.effects) {
				definitionJson["effects"].push_back({
					{ "type", "StatModifier" },
					{ "stat", StatToJsonName(effect.stat) },
					{ "operation", OperationToJsonName(effect.operation) },
					{ "valuePerStack", effect.valuePerStack },
				});
			}
			root["relics"].push_back(std::move(definitionJson));
		}
		return MadoEngine::Json::JsonFile::Save(filePath, root, 2, true);
	}

	bool Catalog::Replace(std::vector<Definition> definitions) {
		if (definitions.empty()) {
			Logger::Output("[Relic] レリック定義が存在しないため置換を中止", Logger::Level::Error);
			return false;
		}

		std::unordered_set<Id> ids;
		for (const Definition& definition : definitions) {
			if (!IsValidDefinition(definition) || !ids.insert(definition.id).second) {
				Logger::Output("[Relic] 不正または重複したレリック定義を検出", Logger::Level::Error);
				return false;
			}
		}

		definitions_ = std::move(definitions);
		return true;
	}

	const Definition* Catalog::Find(const Id& id) const {
		for (const Definition& definition : definitions_) {
			if (definition.id == id) {
				return &definition;
			}
		}
		return nullptr;
	}

}
