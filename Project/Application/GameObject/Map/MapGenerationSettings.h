#pragma once
#include "Math/Vector3.h"
#include <cstdint>
#include <nlohmann/json_fwd.hpp>

/// @brief 地形とイベント配置で共有するMap生成条件
struct MapGenerationSettings {
	static constexpr const char* kFilePath = "Assets/Json/MapGenerator.json";
	static constexpr int kMinimumMapSize = 2;
	static constexpr int kMaximumMapSize = 128;
	static constexpr int kMaximumEventObjectCount = 4096;

	uint32_t seed = 0;
	int mapWidth = 20;
	int mapHeight = 20;
	int jarSpawnCount = 100;
	int moneyJarSpawnCount = 50;
	int expJarSpawnCount = 50;
	int chestSpawnCount = 50;
	int normalChestSpawnCount = 25;
	int freeChestSpawnCount = 25;
	int karmaSpawnCount = 50;
	int magnetSpawnCount = 1;
	int anvilSpawnCount = 5;
	int bossSpawnerSpawnCount = 1;
	Vector3 blockSize = { 15.0f, 7.5f, 15.0f };
	int minHeight = 1;
	int maxHeight = 10;
	int minStartHeight = 1;
	int maxStartHeight = 2;
	int minRangeHeight = -1;
	int maxRangeHeight = 1;
	float slopeSpawnRate = 1.0f;

	/// @brief 編集値を有効範囲に補正
	void Clamp();

	/// @brief 既存の地形生成規則に合わせて適用値を補正
	void ClampForGeneration();

	/// @brief 二つの生成設定の一致を判定
	/// @param rhs 比較対象の設定
	/// @return 全設定が一致する場合はtrue
	bool IsEqualTo(const MapGenerationSettings& rhs) const;

	/// @brief 生成設定をJsonへ変換
	/// @return 生成設定を格納したJson
	nlohmann::json ToJson() const;

	/// @brief Jsonから生成設定を復元
	/// @param json 読み込み元Json
	/// @return 有効な設定を読み込めた場合はtrue
	bool LoadFromJson(const nlohmann::json& json);

	/// @brief 生成設定を設定ファイルへ保存
	/// @return 保存に成功した場合はtrue
	bool SaveToFile() const;

	/// @brief 設定ファイルから生成設定を読み込み
	/// @param useSavedSeed 保存済みシード値も適用する場合はtrue
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(bool useSavedSeed);
};
