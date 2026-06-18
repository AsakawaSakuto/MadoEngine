#pragma once
#include <cstdint>

/// @brief ライトの影響先を分類するレイヤー
enum class LightLayer : uint32_t {
	None = 0,
	World = 1u << 0,
	Player = 1u << 1,
	Enemy = 1u << 2,
	Effect = 1u << 3,
	UI = 1u << 4,
	All = 0xffffffffu,
};

using LightLayerMask = uint32_t;

/// @brief LightLayerをビットマスクへ変換する
/// @param layer 変換するライトレイヤー
/// @return ライトレイヤーのビットマスク
constexpr LightLayerMask ToLightLayerMask(LightLayer layer) {
	return static_cast<LightLayerMask>(layer);
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
