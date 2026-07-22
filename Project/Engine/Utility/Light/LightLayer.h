#pragma once
#include <cstdint>

/// @brief ライトの影響先を分類するレイヤー
enum class LightLayer : uint32_t {
	World,
	Player,
	Enemy,
	Effect,
	UI,

	None = 0xffffffffu,
	All = 0xfffffffeu,
};

using LightLayerMask = uint32_t;

inline constexpr uint32_t kLightLayerCount = static_cast<uint32_t>(LightLayer::UI) + 1;

inline constexpr const char* kLightLayerNames[] = {
	"World",
	"Player",
	"Enemy",
	"Effect",
	"UI",
};

static_assert(kLightLayerCount == sizeof(kLightLayerNames) / sizeof(kLightLayerNames[0]));

/// @brief インデックスからLightLayerを取得する
/// @param index 取得するLightLayerのインデックス
/// @return インデックスに対応するLightLayer
constexpr LightLayer GetLightLayerByIndex(uint32_t index) {
	return static_cast<LightLayer>(index);
}

/// @brief LightLayerが有効な実レイヤーか確認する
/// @param layer 確認するライトレイヤー
/// @return 実レイヤーの場合はtrue
constexpr bool IsValidLightLayer(LightLayer layer) {
	return static_cast<uint32_t>(layer) < kLightLayerCount;
}

inline constexpr LightLayerMask kAllLightLayers = 0xffffffffu;

/// @brief LightLayerをビットマスクへ変換する
/// @param layer 変換するライトレイヤー
/// @return ライトレイヤーのビットマスク
constexpr LightLayerMask ToLightLayerMask(LightLayer layer) {
	if (layer == LightLayer::None) {
		return 0;
	}

	if (layer == LightLayer::All) {
		return kAllLightLayers;
	}

	if (!IsValidLightLayer(layer)) {
		return 0;
	}

	return 1u << static_cast<uint32_t>(layer);
}

/// @brief LightLayerの表示名を取得する
/// @param layer 表示名を取得するライトレイヤー
/// @return ライトレイヤーの表示名。未定義の場合はUnknown
constexpr const char* GetLightLayerName(LightLayer layer) {
	if (layer == LightLayer::None) {
		return "None";
	}

	if (layer == LightLayer::All) {
		return "All";
	}

	if (!IsValidLightLayer(layer)) {
		return "Unknown";
	}

	return kLightLayerNames[static_cast<uint32_t>(layer)];
}

/// @brief LightLayerMaskの表示名を取得する
/// @param layerMask 表示名を取得するライトレイヤーマスク
/// @return ライトレイヤーマスクの表示名。未定義の組み合わせの場合はCustom
constexpr const char* GetLightLayerMaskName(LightLayerMask layerMask) {
	if (layerMask == ToLightLayerMask(LightLayer::None)) {
		return "None";
	}

	if (layerMask == kAllLightLayers) {
		return "All";
	}

	for (uint32_t index = 0; index < kLightLayerCount; ++index) {
		const LightLayer layer = GetLightLayerByIndex(index);
		if (layerMask == ToLightLayerMask(layer)) {
			return GetLightLayerName(layer);
		}
	}

	return "Custom";
}

/// @brief ライトレイヤー同士のOR演算を行う
/// @param left 左辺のライトレイヤー
/// @param right 右辺のライトレイヤー
/// @return 合成したライトレイヤーマスク
constexpr LightLayerMask operator|(LightLayer left, LightLayer right) {
	return ToLightLayerMask(left) | ToLightLayerMask(right);
}

/// @brief ライトレイヤーマスクにライトレイヤーを追加する
/// @param left 左辺のライトレイヤーマスク
/// @param right 追加するライトレイヤー
/// @return 合成したライトレイヤーマスク
constexpr LightLayerMask operator|(LightLayerMask left, LightLayer right) {
	return left | ToLightLayerMask(right);
}
