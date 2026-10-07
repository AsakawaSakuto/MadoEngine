#pragma once
#include <array>
#include <cstddef>
#include <string_view>

/// @brief アイテムと武器アップグレードで共有するレアリティ
enum class Rarity {
	Uncommon = 0,
	Rare,
	Epic,
	Legendary,
	Unique,
};

using RarityColor = std::array<float, 4>;

/// @brief 一種類のレアリティに紐づく共通定義
struct RarityDefinition {
	Rarity rarity = Rarity::Uncommon;
	const char* serializedName = "";
	const char* displayName = "不明";
	RarityColor displayColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	int selectionWeight = 0;
};

// 表示と抽選で参照するレアリティ情報の単一定義
inline constexpr std::array<RarityDefinition, 5> kRarityDefinitions = {
	RarityDefinition{ Rarity::Uncommon,  "Uncommon",  "アンコモン",    { 0.20f, 0.85f, 0.30f, 1.0f }, 60 },
	RarityDefinition{ Rarity::Rare,      "Rare",      "レア",         { 0.20f, 0.55f, 1.00f, 1.0f }, 25 },
	RarityDefinition{ Rarity::Epic,      "Epic",      "エピック",      { 0.70f, 0.30f, 1.00f, 1.0f }, 10 },
	RarityDefinition{ Rarity::Legendary, "Legendary", "レジェンダリー", { 1.00f, 0.85f, 0.15f, 1.0f }, 5 },
	RarityDefinition{ Rarity::Unique,    "Unique",    "ユニーク",      { 1.00f, 0.30f, 0.30f, 1.0f }, 0 },
};

inline constexpr std::size_t kRarityCount = kRarityDefinitions.size();
inline constexpr RarityColor kInvalidRarityColor = { 1.0f, 1.0f, 1.0f, 1.0f };

/// @brief レアリティに対応する共通定義を検索
/// @param rarity 検索対象のレアリティ
/// @return 定義が存在する場合はconstポインター、存在しない場合はnullptr
inline constexpr const RarityDefinition* FindRarityDefinition(Rarity rarity) {
	for (const RarityDefinition& definition : kRarityDefinitions) {
		if (definition.rarity == rarity) {
			return &definition;
		}
	}
	return nullptr;
}

/// @brief レアリティの表示名を取得
/// @param rarity 取得対象のレアリティ
/// @return 表示名、未定義の場合は不明
inline constexpr const char* GetRarityDisplayName(Rarity rarity) {
	const RarityDefinition* definition = FindRarityDefinition(rarity);
	return definition ? definition->displayName : "不明";
}

/// @brief レアリティのシリアライズ名を取得
/// @param rarity 取得対象のレアリティ
/// @return シリアライズ名、未定義の場合は空文字列
inline constexpr const char* GetRaritySerializedName(Rarity rarity) {
	const RarityDefinition* definition = FindRarityDefinition(rarity);
	return definition ? definition->serializedName : "";
}

/// @brief レアリティのデフォルト表示色を取得
/// @param rarity 取得対象のレアリティ
/// @return RGBA形式の表示色
inline constexpr const RarityColor& GetRarityDefaultColor(Rarity rarity) {
	const RarityDefinition* definition = FindRarityDefinition(rarity);
	return definition ? definition->displayColor : kInvalidRarityColor;
}

/// @brief レアリティの抽選ウェイトを取得
/// @param rarity 取得対象のレアリティ
/// @return 抽選ウェイト、抽選対象外または未定義の場合は0
inline constexpr int GetRaritySelectionWeight(Rarity rarity) {
	const RarityDefinition* definition = FindRarityDefinition(rarity);
	return definition ? definition->selectionWeight : 0;
}

/// @brief レアリティが通常抽選の対象か確認
/// @param rarity 確認対象のレアリティ
/// @return 通常抽選へ参加する場合はtrue
inline constexpr bool IsStandardRarity(Rarity rarity) {
	return GetRaritySelectionWeight(rarity) > 0;
}

/// @brief シリアライズ名からレアリティへ変換
/// @param text 変換対象文字列
/// @param outRarity 変換結果の出力先
/// @return 変換に成功した場合はtrue
inline constexpr bool TryParseRarity(std::string_view text, Rarity& outRarity) {
	for (const RarityDefinition& definition : kRarityDefinitions) {
		if (text == definition.serializedName) {
			outRarity = definition.rarity;
			return true;
		}
	}
	return false;
}
