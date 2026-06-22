#pragma once
#include <cstdint>
#include <string>

/// @brief シーンの種類を表す列挙型
enum class SceneType {
	None = -1, // 全シーン共通（特定シーン指定なし）

	Title = 0,
	Game,
	Result,

	Test,

	Count // シーンの総数
};

inline constexpr uint32_t kSceneTypeCount = static_cast<uint32_t>(SceneType::Count);

/// @brief インデックスからSceneTypeを取得する
/// @param index 取得するSceneTypeのインデックス
/// @return インデックスに対応するSceneType
constexpr SceneType GetSceneTypeByIndex(uint32_t index) {
	return static_cast<SceneType>(index);
}

/// @brief SceneType を文字列に変換する
/// @param type シーンの種類
/// @return シーン名文字列
inline std::string SceneTypeToString(SceneType type) {
	switch (type) {
	case SceneType::None:   return "";
	case SceneType::Title:  return "Title";
	case SceneType::Game:   return "Game";
	case SceneType::Result: return "Result";
	case SceneType::Test:   return "Test";
	default:                return "Unknown";
	}
}
