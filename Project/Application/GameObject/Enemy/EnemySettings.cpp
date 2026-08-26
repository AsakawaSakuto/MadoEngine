#include "EnemySettings.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

namespace {
	constexpr const char* kEnemySettingsJsonPath = "Assets/Json/Enemy/EnemySettings.json";
	constexpr float kMinScaleMultiplier = 0.01f;
	constexpr float kMinColliderRadius = 0.01f;
	constexpr float kMinSpawnInterval = 0.01f;
	constexpr float kDefaultWaveDuration = 60.0f;

	/// @brief 非有限値を代替値へ置換
	/// @param value 検証する値
	/// @param fallback 非有限値の場合に使用する値
	/// @return 有限値または代替値
	float SanitizeFinite(float value, float fallback) {
		return std::isfinite(value) ? value : fallback;
	}

	/// @brief Vector3の各成分を有限値へ補正
	/// @param value 補正する値
	/// @param fallback 非有限成分に使用する値
	/// @return 補正済みVector3
	Vector3 SanitizeVector3(const Vector3& value, const Vector3& fallback) {
		return {
			SanitizeFinite(value.x, fallback.x),
			SanitizeFinite(value.y, fallback.y),
			SanitizeFinite(value.z, fallback.z),
		};
	}

	/// @brief Vector4の各成分を有限値へ補正
	/// @param value 補正する値
	/// @param fallback 非有限成分に使用する値
	/// @return 補正済みVector4
	Vector4 SanitizeVector4(const Vector4& value, const Vector4& fallback) {
		return {
			SanitizeFinite(value.x, fallback.x),
			SanitizeFinite(value.y, fallback.y),
			SanitizeFinite(value.z, fallback.z),
			SanitizeFinite(value.w, fallback.w),
		};
	}

	/// @brief Enemy種類設定をJsonへ変換
	/// @param settings 変換するEnemy種類設定
	/// @return 変換済みJson
	nlohmann::json SerializeTypeSettings(const Enemy::TypeSettings& settings) {
		return {
			{ "health", settings.status.currentHealth },
			{ "power", settings.status.power },
			{ "moveSpeed", settings.status.moveSpeed },
			{ "scaleMultiplier", settings.scaleMultiplier },
			{ "movementColliderRadius", settings.movementColliderRadius },
			{ "hitboxMin", MadoEngine::Json::JsonSerializer::ToJson(settings.hitboxMin) },
			{ "hitboxMax", MadoEngine::Json::JsonSerializer::ToJson(settings.hitboxMax) },
			{ "color", MadoEngine::Json::JsonSerializer::ToJson(settings.color) },
		};
	}

	/// @brief JsonからEnemy種類設定を部分更新
	/// @param json 読み込み元Json
	/// @param outSettings 更新先Enemy種類設定
	void DeserializeTypeSettings(const nlohmann::json& json, Enemy::TypeSettings& outSettings) {
		using MadoEngine::Json::JsonSerializer;
		outSettings.status.currentHealth = JsonSerializer::GetOrDefault<float>(
			json, "health", outSettings.status.currentHealth);
		outSettings.status.power = JsonSerializer::GetOrDefault<float>(json, "power", outSettings.status.power);
		outSettings.status.moveSpeed = JsonSerializer::GetOrDefault<float>(
			json, "moveSpeed", outSettings.status.moveSpeed);
		outSettings.scaleMultiplier = JsonSerializer::GetOrDefault<float>(
			json, "scaleMultiplier", outSettings.scaleMultiplier);
		outSettings.movementColliderRadius = JsonSerializer::GetOrDefault<float>(
			json, "movementColliderRadius", outSettings.movementColliderRadius);
		if (json.is_object() && json.contains("hitboxMin")) {
			outSettings.hitboxMin = JsonSerializer::ToVector3(json.at("hitboxMin"), outSettings.hitboxMin);
		}
		if (json.is_object() && json.contains("hitboxMax")) {
			outSettings.hitboxMax = JsonSerializer::ToVector3(json.at("hitboxMax"), outSettings.hitboxMax);
		}
		if (json.is_object() && json.contains("color")) {
			outSettings.color = JsonSerializer::ToVector4(json.at("color"), outSettings.color);
		}
	}

	/// @brief Elite設定をJsonへ変換
	/// @param settings 変換するElite設定
	/// @return 変換済みJson
	nlohmann::json SerializeEliteSettings(const Enemy::EliteSettings& settings) {
		return {
			{ "healthMultiplier", settings.healthMultiplier },
			{ "powerMultiplier", settings.powerMultiplier },
			{ "bodyScaleMultiplier", settings.bodyScaleMultiplier },
			{ "markerHeightOffset", settings.markerHeightOffset },
			{ "markerScale", MadoEngine::Json::JsonSerializer::ToJson(settings.markerScale) },
		};
	}

	/// @brief JsonからElite設定を部分更新
	/// @param json 読み込み元Json
	/// @param outSettings 更新先Elite設定
	void DeserializeEliteSettings(const nlohmann::json& json, Enemy::EliteSettings& outSettings) {
		using MadoEngine::Json::JsonSerializer;
		outSettings.healthMultiplier = JsonSerializer::GetOrDefault<float>(
			json, "healthMultiplier", outSettings.healthMultiplier);
		outSettings.powerMultiplier = JsonSerializer::GetOrDefault<float>(
			json, "powerMultiplier", outSettings.powerMultiplier);
		outSettings.bodyScaleMultiplier = JsonSerializer::GetOrDefault<float>(
			json, "bodyScaleMultiplier", outSettings.bodyScaleMultiplier);
		outSettings.markerHeightOffset = JsonSerializer::GetOrDefault<float>(
			json, "markerHeightOffset", outSettings.markerHeightOffset);
		if (json.is_object() && json.contains("markerScale")) {
			outSettings.markerScale = JsonSerializer::ToVector3(json.at("markerScale"), outSettings.markerScale);
		}
	}

	/// @brief Wave共通生成設定をJsonへ変換
	/// @param settings 変換するWave共通生成設定
	/// @return 変換済みJson
	nlohmann::json SerializeWaveSpawnSettings(const Enemy::WaveSpawnSettings& settings) {
		return {
			{ "spawnInterval", settings.spawnInterval },
			{ "spawnCount", settings.spawnCount },
			{ "eliteSpawnInterval", settings.eliteSpawnInterval },
			{ "minSpawnRadius", settings.minSpawnRadius },
			{ "maxSpawnRadius", settings.maxSpawnRadius },
			{ "normalSpawnRate", settings.normalSpawnRate },
			{ "runnerSpawnRate", settings.runnerSpawnRate },
			{ "tankSpawnRate", settings.tankSpawnRate },
			{ "healthPowerGrowthRatePerMinute", settings.healthPowerGrowthRatePerMinute },
			{ "moveSpeedGrowthRatePerMinute", settings.moveSpeedGrowthRatePerMinute },
		};
	}

	/// @brief Wave設定をJsonへ変換
	/// @param settings 変換するWave設定
	/// @return 変換済みJson
	nlohmann::json SerializeWaveSettings(const Enemy::WaveSettings& settings) {
		nlohmann::json json = SerializeWaveSpawnSettings(settings);
		json["name"] = settings.name;
		json["startTime"] = settings.startTime;
		json["endTime"] = settings.endTime;
		return json;
	}

	/// @brief JsonからWave共通生成設定を部分更新
	/// @param json 読み込み元Json
	/// @param outSettings 更新先Wave共通生成設定
	void DeserializeWaveSpawnSettings(const nlohmann::json& json, Enemy::WaveSpawnSettings& outSettings) {
		using MadoEngine::Json::JsonSerializer;
		outSettings.spawnInterval = JsonSerializer::GetOrDefault<float>(
			json, "spawnInterval", outSettings.spawnInterval);
		outSettings.spawnCount = JsonSerializer::GetOrDefault<std::uint32_t>(
			json, "spawnCount", outSettings.spawnCount);
		outSettings.eliteSpawnInterval = JsonSerializer::GetOrDefault<std::uint32_t>(
			json, "eliteSpawnInterval", outSettings.eliteSpawnInterval);
		outSettings.minSpawnRadius = JsonSerializer::GetOrDefault<float>(
			json, "minSpawnRadius", outSettings.minSpawnRadius);
		outSettings.maxSpawnRadius = JsonSerializer::GetOrDefault<float>(
			json, "maxSpawnRadius", outSettings.maxSpawnRadius);
		outSettings.normalSpawnRate = JsonSerializer::GetOrDefault<float>(
			json, "normalSpawnRate", outSettings.normalSpawnRate);
		outSettings.runnerSpawnRate = JsonSerializer::GetOrDefault<float>(
			json, "runnerSpawnRate", outSettings.runnerSpawnRate);
		outSettings.tankSpawnRate = JsonSerializer::GetOrDefault<float>(
			json, "tankSpawnRate", outSettings.tankSpawnRate);
		outSettings.healthPowerGrowthRatePerMinute = JsonSerializer::GetOrDefault<float>(
			json, "healthPowerGrowthRatePerMinute", outSettings.healthPowerGrowthRatePerMinute);
		outSettings.moveSpeedGrowthRatePerMinute = JsonSerializer::GetOrDefault<float>(
			json, "moveSpeedGrowthRatePerMinute", outSettings.moveSpeedGrowthRatePerMinute);
	}

	/// @brief JsonからWave設定を部分更新
	/// @param json 読み込み元Json
	/// @param outSettings 更新先Wave設定
	void DeserializeWaveSettings(const nlohmann::json& json, Enemy::WaveSettings& outSettings) {
		using MadoEngine::Json::JsonSerializer;
		DeserializeWaveSpawnSettings(json, outSettings);
		outSettings.name = JsonSerializer::GetOrDefault<std::string>(json, "name", outSettings.name);
		outSettings.startTime = JsonSerializer::GetOrDefault<float>(json, "startTime", outSettings.startTime);
		outSettings.endTime = JsonSerializer::GetOrDefault<float>(json, "endTime", outSettings.endTime);
	}

	/// @brief Enemy種類設定を実行可能な範囲へ補正
	/// @param settings 補正するEnemy種類設定
	void NormalizeTypeSettings(Enemy::TypeSettings& settings) {
		settings.status.currentHealth = std::max(0.0f, SanitizeFinite(settings.status.currentHealth, 0.0f));
		settings.status.power = std::max(0.0f, SanitizeFinite(settings.status.power, 0.0f));
		settings.status.moveSpeed = std::max(0.0f, SanitizeFinite(settings.status.moveSpeed, 0.0f));
		settings.scaleMultiplier = std::max(
			kMinScaleMultiplier, SanitizeFinite(settings.scaleMultiplier, kMinScaleMultiplier));
		settings.movementColliderRadius = std::max(
			kMinColliderRadius, SanitizeFinite(settings.movementColliderRadius, kMinColliderRadius));
		settings.hitboxMin = SanitizeVector3(settings.hitboxMin, { -0.5f, 0.0f, -0.5f });
		settings.hitboxMax = SanitizeVector3(settings.hitboxMax, { 0.5f, 1.0f, 0.5f });
		settings.color = SanitizeVector4(settings.color, { 1.0f, 1.0f, 1.0f, 1.0f });
		for (std::size_t channel = 0; channel < 4; ++channel) {
			settings.color[channel] = std::clamp(settings.color[channel], 0.0f, 1.0f);
		}
		for (std::size_t axis = 0; axis < 3; ++axis) {
			if (settings.hitboxMin[axis] > settings.hitboxMax[axis]) {
				std::swap(settings.hitboxMin[axis], settings.hitboxMax[axis]);
			}
		}
	}

	/// @brief Wave共通生成設定を実行可能な範囲へ補正
	/// @param settings 補正するWave共通生成設定
	void NormalizeWaveSpawnSettings(Enemy::WaveSpawnSettings& settings) {
		settings.spawnInterval = std::max(
			kMinSpawnInterval, SanitizeFinite(settings.spawnInterval, kMinSpawnInterval));
		settings.spawnCount = std::max<std::uint32_t>(1, settings.spawnCount);
		settings.eliteSpawnInterval = std::max<std::uint32_t>(1, settings.eliteSpawnInterval);
		settings.minSpawnRadius = std::max(0.0f, SanitizeFinite(settings.minSpawnRadius, 0.0f));
		settings.maxSpawnRadius = std::max(
			settings.minSpawnRadius, SanitizeFinite(settings.maxSpawnRadius, settings.minSpawnRadius));
		settings.normalSpawnRate = std::max(0.0f, SanitizeFinite(settings.normalSpawnRate, 0.0f));
		settings.runnerSpawnRate = std::max(0.0f, SanitizeFinite(settings.runnerSpawnRate, 0.0f));
		settings.tankSpawnRate = std::max(0.0f, SanitizeFinite(settings.tankSpawnRate, 0.0f));
		if (settings.normalSpawnRate + settings.runnerSpawnRate + settings.tankSpawnRate <= 0.0f) {
			settings.normalSpawnRate = 1.0f;
		}
		settings.healthPowerGrowthRatePerMinute = std::max(
			0.0f, SanitizeFinite(settings.healthPowerGrowthRatePerMinute, 0.0f));
		settings.moveSpeedGrowthRatePerMinute = std::max(
			0.0f, SanitizeFinite(settings.moveSpeedGrowthRatePerMinute, 0.0f));
	}

	/// @brief 通常Wave設定を実行可能な範囲へ補正
	/// @param settings 補正する通常Wave設定
	void NormalizeWaveSettings(Enemy::WaveSettings& settings) {
		NormalizeWaveSpawnSettings(settings);
		settings.startTime = std::max(0.0f, SanitizeFinite(settings.startTime, 0.0f));
		settings.endTime = std::max(settings.startTime, SanitizeFinite(settings.endTime, settings.startTime));
		if (settings.name.empty()) {
			settings.name = "Wave";
		}
	}
} // namespace

namespace Enemy {

	Settings& Settings::GetInstance() {
		static Settings instance;
		return instance;
	}

	Settings::Settings() {
		ResetToDefaults();
	}

	bool Settings::LoadOrCreate() {
		if (MadoEngine::Json::JsonFile::Exists(kEnemySettingsJsonPath)) {
			return Load();
		}

		ResetToDefaults();
		Logger::Output("Enemy設定Jsonが存在しないため既定値で作成します", Logger::Level::Assets);
		return Save();
	}

	bool Settings::Load() {
		nlohmann::json root;
		std::uint32_t legacyEliteSpawnInterval = 50;
		ResetToDefaults();
		if (!MadoEngine::Json::JsonFile::Load(kEnemySettingsJsonPath, root) || !root.is_object()) {
			return false;
		}

		// 欠落項目だけ既定値を維持できるよう存在する設定だけを部分反映
		if (root.contains("enemyStatus") && root.at("enemyStatus").is_object()) {
			const nlohmann::json& statusJson = root.at("enemyStatus");
			for (const Data::Type type : { Data::Type::Normal, Data::Type::Runner, Data::Type::Tank, Data::Type::Boss }) {
				const char* typeName = EnemyTypeToString(type);
				if (statusJson.contains(typeName)) {
					DeserializeTypeSettings(statusJson.at(typeName), EditTypeSettings(type));
				}
			}
			if (statusJson.contains("Elite")) {
				const nlohmann::json& eliteJson = statusJson.at("Elite");
				legacyEliteSpawnInterval = MadoEngine::Json::JsonSerializer::GetOrDefault<std::uint32_t>(
					eliteJson, "spawnInterval", legacyEliteSpawnInterval);
				DeserializeEliteSettings(eliteJson, eliteSettings_);
			}
		}

		// EnemySpawner共通設定を読み込んだ後で存在するWave配列だけを置換
		if (root.contains("enemySpawner") && root.at("enemySpawner").is_object()) {
			const nlohmann::json& spawnerJson = root.at("enemySpawner");
			const bool hasBonusWaveSettings =
				spawnerJson.contains("bonusWave") && spawnerJson.at("bonusWave").is_object();
			std::uint64_t maxAliveEnemies = MadoEngine::Json::JsonSerializer::GetOrDefault<std::uint64_t>(
				spawnerJson, "maxAliveEnemies", static_cast<std::uint64_t>(maxAliveEnemies_));

			// 旧形式の先頭Waveにある最大生存数を共通設定へ移行
			if (!spawnerJson.contains("maxAliveEnemies") && spawnerJson.contains("waves") &&
				spawnerJson.at("waves").is_array() && !spawnerJson.at("waves").empty()) {
				maxAliveEnemies = MadoEngine::Json::JsonSerializer::GetOrDefault<std::uint64_t>(
					spawnerJson.at("waves").front(), "spawnLimit", maxAliveEnemies);
			}
			maxAliveEnemies_ = static_cast<std::size_t>(std::min<std::uint64_t>(
				maxAliveEnemies, (std::numeric_limits<std::size_t>::max)()));
			if (hasBonusWaveSettings) {
				DeserializeWaveSpawnSettings(spawnerJson.at("bonusWave"), bonusWaveSettings_);
			}
			if (spawnerJson.contains("waves") && spawnerJson.at("waves").is_array()) {
				waves_.clear();
				for (const nlohmann::json& waveJson : spawnerJson.at("waves")) {
					WaveSettings wave;
					wave.eliteSpawnInterval = legacyEliteSpawnInterval;
					DeserializeWaveSettings(waveJson, wave);
					waves_.push_back(std::move(wave));
				}
			}

			// 旧Jsonでは最終通常Waveの値を引き継いで時間切れ後の生成停止を回避
			if (!hasBonusWaveSettings && !waves_.empty()) {
				bonusWaveSettings_ = static_cast<const WaveSpawnSettings&>(waves_.back());
			}
		}

		Normalize();
		Logger::Output("Enemy設定をJsonから読み込みました", Logger::Level::Assets);
		return true;
	}

	bool Settings::Save() const {

		// 実行時設定と同じ階層構造で種類別設定を直列化
		nlohmann::json statusJson = nlohmann::json::object();
		for (const Data::Type type : { Data::Type::Normal, Data::Type::Runner, Data::Type::Tank, Data::Type::Boss }) {
			statusJson[EnemyTypeToString(type)] = SerializeTypeSettings(GetTypeSettings(type));
		}
		statusJson["Elite"] = SerializeEliteSettings(eliteSettings_);

		// Editor上の並び順を保持したまま全Waveを直列化
		nlohmann::json waveJson = nlohmann::json::array();
		for (const WaveSettings& wave : waves_) {
			waveJson.push_back(SerializeWaveSettings(wave));
		}

		nlohmann::json root;
		root["enemyStatus"] = std::move(statusJson);
		root["enemySpawner"]["maxAliveEnemies"] = maxAliveEnemies_;
		root["enemySpawner"]["bonusWave"] = SerializeWaveSpawnSettings(bonusWaveSettings_);
		root["enemySpawner"]["waves"] = std::move(waveJson);
		const bool wasSaved = MadoEngine::Json::JsonFile::Save(kEnemySettingsJsonPath, root, 4, true);
		if (wasSaved) {
			Logger::Output("Enemy設定をJsonへ保存しました", Logger::Level::Assets);
		}
		return wasSaved;
	}

	void Settings::ResetToDefaults() {

		// 既存ゲームバランスを初回生成時の基準値として維持
		typeSettings_[ToTypeIndex(Data::Type::Normal)] = {
			{ 10.0f, 5.0f, 3.0f }, 0.5f, 0.5f, { -0.5f, 0.0f, -0.5f }, { 0.5f, 2.0f, 0.5f },
			{ 0.85f, 0.18f, 0.18f, 1.0f }
		};
		typeSettings_[ToTypeIndex(Data::Type::Runner)] = {
			{ 5.0f, 3.5f, 5.4f }, 0.35f, 0.35f, { -0.35f, 0.0f, -0.35f }, { 0.35f, 1.4f, 0.35f },
			{ 0.15f, 0.65f, 1.0f, 1.0f }
		};
		typeSettings_[ToTypeIndex(Data::Type::Tank)] = {
			{ 40.0f, 7.5f, 1.65f }, 0.8f, 0.8f, { -0.8f, 0.0f, -0.8f }, { 0.8f, 3.2f, 0.8f },
			{ 0.95f, 0.55f, 0.1f, 1.0f }
		};
		typeSettings_[ToTypeIndex(Data::Type::Boss)] = {
			{ 1000.0f, 20.0f, 1.5f }, 1.5f, 1.5f, { -1.5f, 0.0f, -1.5f }, { 1.5f, 4.0f, 1.5f },
			{ 0.7f, 0.15f, 0.9f, 1.0f }
		};
		eliteSettings_ = {};
		maxAliveEnemies_ = 500;
		waves_ = { WaveSettings{} };
		bonusWaveSettings_ = {};
		bonusWaveSettings_.spawnInterval = 0.15f;
		bonusWaveSettings_.spawnCount = 2;
		bonusWaveSettings_.eliteSpawnInterval = 25;
		bonusWaveSettings_.normalSpawnRate = 0.35f;
		bonusWaveSettings_.runnerSpawnRate = 0.35f;
		bonusWaveSettings_.tankSpawnRate = 0.3f;
		bonusWaveSettings_.healthPowerGrowthRatePerMinute = 0.15f;
		bonusWaveSettings_.moveSpeedGrowthRatePerMinute = 0.03f;
	}

	void Settings::Normalize() {

		// Editor入力やJson改変後もColliderと生成処理が成立する値域へ補正
		for (TypeSettings& settings : typeSettings_) {
			NormalizeTypeSettings(settings);
		}

		eliteSettings_.healthMultiplier = std::max(
			0.0f, SanitizeFinite(eliteSettings_.healthMultiplier, 0.0f));
		eliteSettings_.powerMultiplier = std::max(
			0.0f, SanitizeFinite(eliteSettings_.powerMultiplier, 0.0f));
		eliteSettings_.bodyScaleMultiplier = std::max(
			kMinScaleMultiplier,
			SanitizeFinite(eliteSettings_.bodyScaleMultiplier, kMinScaleMultiplier));
		eliteSettings_.markerHeightOffset = std::max(
			0.0f, SanitizeFinite(eliteSettings_.markerHeightOffset, 0.0f));
		eliteSettings_.markerScale = SanitizeVector3(eliteSettings_.markerScale, { 0.45f, 0.45f, 0.45f });
		eliteSettings_.markerScale.x = std::max(kMinScaleMultiplier, eliteSettings_.markerScale.x);
		eliteSettings_.markerScale.y = std::max(kMinScaleMultiplier, eliteSettings_.markerScale.y);
		eliteSettings_.markerScale.z = std::max(kMinScaleMultiplier, eliteSettings_.markerScale.z);

		for (WaveSettings& wave : waves_) {
			NormalizeWaveSettings(wave);
		}
		NormalizeWaveSpawnSettings(bonusWaveSettings_);
	}

	const TypeSettings& Settings::GetTypeSettings(Data::Type type) const {
		return typeSettings_[ToTypeIndex(type)];
	}

	TypeSettings& Settings::EditTypeSettings(Data::Type type) {
		return typeSettings_[ToTypeIndex(type)];
	}

	void Settings::AddWave() {
		WaveSettings wave;
		if (!waves_.empty()) {
			wave = waves_.back();
			const float previousDuration = std::max(
				kDefaultWaveDuration, waves_.back().endTime - waves_.back().startTime);
			wave.startTime = waves_.back().endTime;
			wave.endTime = wave.startTime + previousDuration;
		}
		wave.name = "Wave " + std::to_string(waves_.size() + 1);
		waves_.push_back(std::move(wave));
	}

	bool Settings::RemoveWave(std::size_t index) {
		if (index >= waves_.size()) {
			return false;
		}

		waves_.erase(waves_.begin() + static_cast<std::ptrdiff_t>(index));
		return true;
	}

	std::size_t Settings::ToTypeIndex(Data::Type type) {
		switch (type) {
		case Data::Type::Normal: return 0;
		case Data::Type::Runner: return 1;
		case Data::Type::Tank:   return 2;
		case Data::Type::Boss:   return 3;
		}
		return 0;
	}

	const char* EnemyTypeToString(Data::Type type) {
		switch (type) {
		case Data::Type::Normal: return "Normal";
		case Data::Type::Runner: return "Runner";
		case Data::Type::Tank:   return "Tank";
		case Data::Type::Boss:   return "Boss";
		}
		return "Normal";
	}

} // namespace Enemy
