#include "MapGenerationSettings.h"
#include "Utility/Json/Core/JsonFile.h"
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace {
/// @brief 保存名と種別ごとの設定メンバーの対応
struct EventObjectSettingsMember {
	const char* name;
	MapEventObjectSettings MapGenerationSettings::* member;
	const char* legacyName = nullptr;
	const char* fallbackLegacyName = nullptr;
};

constexpr EventObjectSettingsMember kEventObjectSettingsMembers[] = {
	{ "jar", &MapGenerationSettings::jarSettings, "moneyJar", "expJar" },
	{ "chest", &MapGenerationSettings::chestSettings, "normalChest", "freeChest" },
	{ "karma", &MapGenerationSettings::karmaSettings },
	{ "magnet", &MapGenerationSettings::magnetSettings },
	{ "anvil", &MapGenerationSettings::anvilSettings },
	{ "bossSpawner", &MapGenerationSettings::bossSpawnerSettings },
};

/// @brief 保存名と装飾種別ごとの遮蔽判定設定メンバーの対応
struct DecorationObjectSettingsMember {
	const char* name;
	MapDecorationObjectSettings MapGenerationSettings::* member;
};

constexpr DecorationObjectSettingsMember kDecorationObjectSettingsMembers[] = {
	{ "tree", &MapGenerationSettings::treeSettings },
	{ "rock", &MapGenerationSettings::rockSettings }
};

/// @brief 指定されたJsonの三成分配列を復元
/// @param json 読み込み元Json
/// @param key 三成分配列の保存名
/// @param value 復元先のベクトル
/// @return 未指定または有効な三成分配列の場合はtrue
bool LoadOptionalVector3(const nlohmann::json& json, const char* key, Vector3& value) {
	const auto it = json.find(key);
	if (it == json.end()) {
		return true;
	}

	// 一部成分だけの更新を拒否してColliderの形状を一組として復元
	if (!it->is_array() || it->size() != 3) {
		return false;
	}
	value = { (*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>() };
	return true;
}
}

void MapGenerationSettings::Clamp() {

	// 配置数の内訳と有限な寸法を先に確定して生成時の不正な範囲を防止
	mapWidth              = std::clamp(mapWidth, kMinimumMapSize, kMaximumMapSize);
	mapHeight             = std::clamp(mapHeight, kMinimumMapSize, kMaximumMapSize);
	jarSpawnCount         = std::clamp(jarSpawnCount, 0, kMaximumEventObjectCount);
	moneyJarSpawnCount    = std::clamp(moneyJarSpawnCount, 0, jarSpawnCount);
	expJarSpawnCount      = jarSpawnCount - moneyJarSpawnCount;
	chestSpawnCount       = std::clamp(chestSpawnCount, 0, kMaximumEventObjectCount);
	normalChestSpawnCount = std::clamp(normalChestSpawnCount, 0, chestSpawnCount);
	freeChestSpawnCount   = chestSpawnCount - normalChestSpawnCount;
	karmaSpawnCount       = std::clamp(karmaSpawnCount, 0, kMaximumEventObjectCount);
	magnetSpawnCount      = std::clamp(magnetSpawnCount, 0, kMaximumEventObjectCount);
	anvilSpawnCount       = std::clamp(anvilSpawnCount, 0, kMaximumEventObjectCount);
	bossSpawnerSpawnCount = std::clamp(bossSpawnerSpawnCount, 0, kMaximumEventObjectCount);
	treeSpawnCount        = std::clamp(treeSpawnCount, 0, kMaximumDecorationObjectCount);
	rockSpawnCount        = std::clamp(rockSpawnCount, 0, kMaximumDecorationObjectCount);

	// 種別を追加した場合も保存・比較と同じ一覧で相互作用設定の補正漏れを防止
	for (const EventObjectSettingsMember& entry : kEventObjectSettingsMembers) {
		(this->*entry.member).Clamp();
	}

	// 装飾の遮蔽範囲も保存・比較と同じ一覧で補正
	for (const DecorationObjectSettingsMember& entry : kDecorationObjectSettingsMembers) {
		(this->*entry.member).Clamp();
	}

	blockSize.x = std::clamp([&]() {
		if (std::isfinite(blockSize.x)) {
			return (blockSize.x);
		} else {
			return (15.0f);
		}
	}(), 0.5f, 100.0f);
	blockSize.y = std::clamp([&]() {
		if (std::isfinite(blockSize.y)) {
			return (blockSize.y);
		} else {
			return (7.5f);
		}
	}(), 0.5f, 100.0f);
	blockSize.z = std::clamp([&]() {
		if (std::isfinite(blockSize.z)) {
			return (blockSize.z);
		} else {
			return (15.0f);
		}
	}(), 0.5f, 100.0f);
	minHeight      = std::clamp(minHeight, 1, 100);
	maxHeight      = std::clamp(maxHeight, minHeight, 100);
	minStartHeight = std::clamp(minStartHeight, minHeight, maxHeight);
	maxStartHeight = std::clamp(maxStartHeight, minStartHeight, maxHeight);
	minRangeHeight = std::clamp(minRangeHeight, -10, 0);
	maxRangeHeight = std::clamp(maxRangeHeight, 0, 10);
	slopeSpawnRate = std::clamp(
		[&]() {
			if (std::isfinite(slopeSpawnRate)) {
				return (slopeSpawnRate);
			} else {
				return (1.0f);
			}
		}(),
		0.0f,
		1.0f
	);
}

void MapGenerationSettings::ClampForGeneration() {
	Clamp();

	// 編集値の0を保存可能にしつつ、適用時は従来の最低一段の高さ変化を維持
	minRangeHeight = std::clamp(minRangeHeight, -10, -1);
	maxRangeHeight = std::clamp(maxRangeHeight, 1, 10);
}

bool MapGenerationSettings::IsEqualTo(const MapGenerationSettings& rhs) const {

	// Colliderや待ち時間だけの編集も未適用表示とUndoの差分として検出
	for (const EventObjectSettingsMember& entry : kEventObjectSettingsMembers) {
		if (!(this->*entry.member).IsEqualTo(rhs.*entry.member)) {
			return false;
		}
	}

	// 装飾の範囲だけの編集も未適用表示とUndoの差分として検出
	for (const DecorationObjectSettingsMember& entry : kDecorationObjectSettingsMembers) {
		if (!(this->*entry.member).IsEqualTo(rhs.*entry.member)) {
			return false;
		}
	}

	return seed == rhs.seed &&
		mapWidth == rhs.mapWidth &&
		mapHeight == rhs.mapHeight &&
		jarSpawnCount == rhs.jarSpawnCount &&
		moneyJarSpawnCount == rhs.moneyJarSpawnCount &&
		expJarSpawnCount == rhs.expJarSpawnCount &&
		chestSpawnCount == rhs.chestSpawnCount &&
		normalChestSpawnCount == rhs.normalChestSpawnCount &&
		freeChestSpawnCount == rhs.freeChestSpawnCount &&
		karmaSpawnCount == rhs.karmaSpawnCount &&
		magnetSpawnCount == rhs.magnetSpawnCount &&
		anvilSpawnCount == rhs.anvilSpawnCount &&
		bossSpawnerSpawnCount == rhs.bossSpawnerSpawnCount &&
		treeSpawnCount == rhs.treeSpawnCount &&
		rockSpawnCount == rhs.rockSpawnCount &&
		blockSize.x == rhs.blockSize.x &&
		blockSize.y == rhs.blockSize.y &&
		blockSize.z == rhs.blockSize.z &&
		minHeight == rhs.minHeight &&
		maxHeight == rhs.maxHeight &&
		minStartHeight == rhs.minStartHeight &&
		maxStartHeight == rhs.maxStartHeight &&
		minRangeHeight == rhs.minRangeHeight &&
		maxRangeHeight == rhs.maxRangeHeight &&
		slopeSpawnRate == rhs.slopeSpawnRate;
}

nlohmann::json MapGenerationSettings::ToJson() const {
	nlohmann::json json;
	json["seed"] = seed;
	json["mapSize"] = { mapWidth, mapHeight };
	json["eventObjectCounts"] = {
		{ "jar", jarSpawnCount },
		{ "chest", chestSpawnCount },
		{ "karma", karmaSpawnCount },
		{ "magnet", magnetSpawnCount },
		{ "anvil", anvilSpawnCount },
		{ "bossSpawner", bossSpawnerSpawnCount },
		{ "jarTypes", {
			{ "money", moneyJarSpawnCount },
			{ "exp", expJarSpawnCount }
		} },
		{ "chestTypes", {
			{ "normal", normalChestSpawnCount },
			{ "free", freeChestSpawnCount }
		} }
	};
	json["blockSize"] = { blockSize.x, blockSize.y, blockSize.z };
	json["decorationObjectCounts"] = {
		{ "tree", treeSpawnCount },
		{ "rock", rockSpawnCount }
	};

	// 配置数とは独立した設定として種別ごとの接触範囲と待ち時間を保存
	for (const EventObjectSettingsMember& entry : kEventObjectSettingsMembers) {
		const MapEventObjectSettings& settings = this->*entry.member;
		json["eventObjectSettings"][entry.name] = {
			{ "autoInteractionDuration", settings.autoInteractionDuration },
			{ "aabbMin", { settings.colliderMin.x, settings.colliderMin.y, settings.colliderMin.z } },
			{ "aabbMax", { settings.colliderMax.x, settings.colliderMax.y, settings.colliderMax.z } },
		};
	}

	// 配置数とは独立した遮蔽判定範囲を装飾種別ごとに保存
	for (const DecorationObjectSettingsMember& entry : kDecorationObjectSettingsMembers) {
		const MapDecorationObjectSettings& settings = this->*entry.member;
		json["decorationObjectSettings"][entry.name] = {
			{ "aabbMin", { settings.colliderMin.x, settings.colliderMin.y, settings.colliderMin.z } },
			{ "aabbMax", { settings.colliderMax.x, settings.colliderMax.y, settings.colliderMax.z } }
		};
	}

	json["height"] = {
		{ "min", minHeight },
		{ "max", maxHeight },
		{ "startMin", minStartHeight },
		{ "startMax", maxStartHeight },
		{ "rangeMin", minRangeHeight },
		{ "rangeMax", maxRangeHeight }
	};
	json["slopeSpawnRate"] = slopeSpawnRate;
	return json;
}

bool MapGenerationSettings::LoadFromJson(const nlohmann::json& json) {
	if (!json.is_object()) {
		return false;
	}

	// 読み込み途中の型不一致で元の設定が部分更新されないよう一時値へ復元
	try {
		MapGenerationSettings settings = *this;
		if (const auto seedIt = json.find("seed"); seedIt != json.end() && seedIt->is_number_unsigned()) {
			settings.seed = seedIt->get<uint32_t>();
		}
		if (const auto sizeIt = json.find("mapSize"); sizeIt != json.end() && sizeIt->is_array() && sizeIt->size() >= 2) {
			settings.mapWidth = (*sizeIt)[0].get<int>();
			settings.mapHeight = (*sizeIt)[1].get<int>();
		}
		if (const auto countIt = json.find("eventObjectCounts"); countIt != json.end() && countIt->is_object()) {
			settings.jarSpawnCount = countIt->value("jar", settings.jarSpawnCount);
			settings.chestSpawnCount = countIt->value("chest", settings.chestSpawnCount);
			settings.karmaSpawnCount = countIt->value("karma", settings.karmaSpawnCount);
			settings.magnetSpawnCount = countIt->value("magnet", settings.magnetSpawnCount);
			settings.anvilSpawnCount = countIt->value("anvil", settings.anvilSpawnCount);
			settings.bossSpawnerSpawnCount = countIt->value("bossSpawner", settings.bossSpawnerSpawnCount);

			// 旧Jsonにタイプ別設定がない場合は総数を均等配分して互換性を維持
			if (const auto jarTypesIt = countIt->find("jarTypes"); jarTypesIt != countIt->end() && jarTypesIt->is_object()) {
				settings.moneyJarSpawnCount = jarTypesIt->value("money", settings.moneyJarSpawnCount);
				settings.expJarSpawnCount = jarTypesIt->value("exp", settings.expJarSpawnCount);
			} else {
				settings.moneyJarSpawnCount = (settings.jarSpawnCount + 1) / 2;
				settings.expJarSpawnCount = settings.jarSpawnCount - settings.moneyJarSpawnCount;
			}
			if (const auto chestTypesIt = countIt->find("chestTypes"); chestTypesIt != countIt->end() && chestTypesIt->is_object()) {
				settings.normalChestSpawnCount = chestTypesIt->value("normal", settings.normalChestSpawnCount);
				settings.freeChestSpawnCount = chestTypesIt->value("free", settings.freeChestSpawnCount);
			} else {
				settings.normalChestSpawnCount = (settings.chestSpawnCount + 1) / 2;
				settings.freeChestSpawnCount = settings.chestSpawnCount - settings.normalChestSpawnCount;
			}
		}

		// 装飾を含まない旧Jsonでは既定の配置数を維持し、型不一致は全体の読込失敗として処理
		if (const auto countIt = json.find("decorationObjectCounts"); countIt != json.end()) {
			if (!countIt->is_object()) {
				return false;
			}
			settings.treeSpawnCount = countIt->value("tree", settings.treeSpawnCount);

			// 新しい総数設定を優先し、旧タイプ別設定だけがある場合は有効な配置数を合算
			if (countIt->contains("rock")) {
				settings.rockSpawnCount = countIt->at("rock").get<int>();
			} else if (countIt->contains("rock0") || countIt->contains("rock1") || countIt->contains("rock2")) {
				int rockCount = 0;
				for (const char* key : { "rock0", "rock1", "rock2" }) {
					rockCount += std::clamp(countIt->value(key, 0), 0, kMaximumDecorationObjectCount);
				}
				settings.rockSpawnCount = rockCount;
			}
		}

		// 旧Jsonに相互作用設定がない場合は従来の時間とCollider寸法を維持
		if (const auto eventSettingsIt = json.find("eventObjectSettings"); eventSettingsIt != json.end()) {
			if (!eventSettingsIt->is_object()) {
				return false;
			}
			for (const EventObjectSettingsMember& entry : kEventObjectSettingsMembers) {
				auto objectIt = eventSettingsIt->find(entry.name);

				// 旧タイプ別設定はMoneyとNormalを優先し、共通設定がある場合はそちらを採用
				if (objectIt == eventSettingsIt->end() && entry.legacyName) {
					objectIt = eventSettingsIt->find(entry.legacyName);
					if (objectIt == eventSettingsIt->end() && entry.fallbackLegacyName) {
						objectIt = eventSettingsIt->find(entry.fallbackLegacyName);
					}
				}
				if (objectIt == eventSettingsIt->end()) {
					continue;
				}
				if (!objectIt->is_object()) {
					return false;
				}
				MapEventObjectSettings& objectSettings = settings.*entry.member;
				objectSettings.autoInteractionDuration = objectIt->value("autoInteractionDuration", objectSettings.autoInteractionDuration);
				if (!LoadOptionalVector3(*objectIt, "aabbMin", objectSettings.colliderMin) ||
					!LoadOptionalVector3(*objectIt, "aabbMax", objectSettings.colliderMax)) {
					return false;
				}
			}
		}

		// 遮蔽設定のない旧Jsonでは既定値を維持し、不正な配列では部分更新を拒否
		if (const auto decorationSettingsIt = json.find("decorationObjectSettings"); decorationSettingsIt != json.end()) {
			if (!decorationSettingsIt->is_object()) {
				return false;
			}
			for (const DecorationObjectSettingsMember& entry : kDecorationObjectSettingsMembers) {
				const auto objectIt = decorationSettingsIt->find(entry.name);
				if (objectIt == decorationSettingsIt->end()) {
					continue;
				}
				if (!objectIt->is_object()) {
					return false;
				}
				MapDecorationObjectSettings& objectSettings = settings.*entry.member;
				if (!LoadOptionalVector3(*objectIt, "aabbMin", objectSettings.colliderMin) ||
					!LoadOptionalVector3(*objectIt, "aabbMax", objectSettings.colliderMax)) {
					return false;
				}
			}
		}

		if (const auto blockSizeIt = json.find("blockSize"); blockSizeIt != json.end() && blockSizeIt->is_array() && blockSizeIt->size() >= 3) {
			settings.blockSize = {
				(*blockSizeIt)[0].get<float>(),
				(*blockSizeIt)[1].get<float>(),
				(*blockSizeIt)[2].get<float>()
			};
		}
		if (const auto heightIt = json.find("height"); heightIt != json.end() && heightIt->is_object()) {
			settings.minHeight = heightIt->value("min", settings.minHeight);
			settings.maxHeight = heightIt->value("max", settings.maxHeight);
			settings.minStartHeight = heightIt->value("startMin", settings.minStartHeight);
			settings.maxStartHeight = heightIt->value("startMax", settings.maxStartHeight);
			settings.minRangeHeight = heightIt->value("rangeMin", settings.minRangeHeight);
			settings.maxRangeHeight = heightIt->value("rangeMax", settings.maxRangeHeight);
		}
		settings.slopeSpawnRate = json.value("slopeSpawnRate", settings.slopeSpawnRate);
		settings.Clamp();
		*this = settings;
		return true;
	} catch (const nlohmann::json::exception&) {
		return false;
	}
}

bool MapGenerationSettings::SaveToFile() const {
	nlohmann::json root;
	root["formatVersion"] = 2;
	root["settings"] = ToJson();
	return MadoEngine::Json::JsonFile::Save(kFilePath, root, 4, true);
}

bool MapGenerationSettings::LoadFromFile(bool useSavedSeed) {
	if (!MadoEngine::Json::JsonFile::Exists(kFilePath)) {
		return false;
	}

	nlohmann::json root;
	if (!MadoEngine::Json::JsonFile::Load(kFilePath, root) || !root.is_object()) {
		return false;
	}

	// 初期化時はScene指定のシードを優先し、手動再読込時だけ保存値を採用
	const uint32_t preservedSeed = seed;
	const auto settingsIt = root.find("settings");
	if (settingsIt == root.end() || !LoadFromJson(*settingsIt)) {
		return false;
	}
	if (!useSavedSeed) {
		seed = preservedSeed;
	}
	return true;
}
