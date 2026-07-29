#pragma once
#include <cstdint>
#include <string>

namespace MadoEngine::Render {

	/// @brief 描画対象を分類するレイヤー
	enum class RenderLayer : uint32_t {
		Default,
		World,
		MapEventObject,
		MapEventObjectOutline,
		Player,
		Effect,
		UI,
		Debug,

		Sphere1,
		Sphere2,
		Sphere3,
		Sphere4,
		Sphere5,
		Sphere6,
		Sphere7,
		Sphere8,
		Sphere9,
	};

	inline constexpr const char* kRenderLayerNames[] = {
		"Default",
		"World",
		"MapEventObject",
		"MapEventObjectOutline",
		"Player",
		"Effect",
		"UI",
		"Debug",

		"Sphere1",
		"Sphere2",
		"Sphere3",
		"Sphere4",
		"Sphere5",
		"Sphere6",
		"Sphere7",
		"Sphere8",
		"Sphere9",
	};

	inline std::string RenderLayerToString(RenderLayer layer) {
		switch (layer) {
		case RenderLayer::World:
			return "World";
		case RenderLayer::MapEventObject:
			return "MapEventObject";
		case RenderLayer::MapEventObjectOutline:
			return "MapEventObjectOutline";
		case RenderLayer::Player:
			return "Player";
		case RenderLayer::Effect:
			return "Effect";
		case RenderLayer::UI:
			return "UI";
		case RenderLayer::Debug:
			return "Debug";
		case RenderLayer::Default:

		case RenderLayer::Sphere1:
			return "Sphere1";
		case RenderLayer::Sphere2:
			return "Sphere2";
		case RenderLayer::Sphere3:
			return "Sphere3";
		case RenderLayer::Sphere4:
			return "Sphere4";
		case RenderLayer::Sphere5:
			return "Sphere5";
		case RenderLayer::Sphere6:
			return "Sphere6";
		case RenderLayer::Sphere7:
			return "Sphere7";
		case RenderLayer::Sphere8:
			return "Sphere8";
		case RenderLayer::Sphere9:
			return "Sphere9";

		default:
			return "Default";
		}
	}

	inline RenderLayer RenderLayerFromString(const std::string& value) {
		if (value == "World") { return RenderLayer::World; }
		if (value == "MapEventObject") { return RenderLayer::MapEventObject; }
		if (value == "MapEventObjectOutline") { return RenderLayer::MapEventObjectOutline; }
		if (value == "Player") { return RenderLayer::Player; }
		if (value == "Effect") { return RenderLayer::Effect; }
		if (value == "UI") { return RenderLayer::UI; }
		if (value == "Debug") { return RenderLayer::Debug; }

		if (value == "Sphere1") { return RenderLayer::Sphere1; }
		if (value == "Sphere2") { return RenderLayer::Sphere2; }
		if (value == "Sphere3") { return RenderLayer::Sphere3; }
		if (value == "Sphere4") { return RenderLayer::Sphere4; }
		if (value == "Sphere5") { return RenderLayer::Sphere5; }
		if (value == "Sphere6") { return RenderLayer::Sphere6; }
		if (value == "Sphere7") { return RenderLayer::Sphere7; }
		if (value == "Sphere8") { return RenderLayer::Sphere8; }
		if (value == "Sphere9") { return RenderLayer::Sphere9; }
		return RenderLayer::Default;
	}

	using RenderLayerMask = uint32_t;

	inline constexpr uint32_t kRenderLayerCount =
		static_cast<uint32_t>(sizeof(kRenderLayerNames) / sizeof(kRenderLayerNames[0]));

	static_assert(kRenderLayerCount == sizeof(kRenderLayerNames) / sizeof(kRenderLayerNames[0]));

	/// @brief インデックスからRenderLayerを取得する
	/// @param index 取得するRenderLayerのインデックス
	/// @return インデックスに対応するRenderLayer
	inline constexpr RenderLayer GetRenderLayerByIndex(uint32_t index) {
		return static_cast<RenderLayer>(index);
	}

	/// @brief RenderLayerが有効な実レイヤーか確認する
	/// @param layer 確認するレイヤー
	/// @return 実レイヤーの場合はtrue
	inline constexpr bool IsValidRenderLayer(RenderLayer layer) {
		return static_cast<uint32_t>(layer) < kRenderLayerCount;
	}

	/// @brief RenderLayerをマスク値へ変換する
	/// @param layer 変換対象のレイヤー
	/// @return 指定レイヤーのみを含むマスク
	inline constexpr RenderLayerMask ToRenderLayerMask(RenderLayer layer) {
		if (!IsValidRenderLayer(layer)) {
			return 0;
		}

		return 1u << static_cast<uint32_t>(layer);
	}

	/// @brief 定義済みRenderLayerをすべて含むマスクを作成する
	/// @return 定義済みRenderLayerをすべて含むマスク
	inline constexpr RenderLayerMask BuildAllRenderLayerMask() {
		RenderLayerMask mask = 0;
		for (uint32_t index = 0; index < kRenderLayerCount; ++index) {
			mask |= ToRenderLayerMask(GetRenderLayerByIndex(index));
		}

		return mask;
	}

	inline constexpr RenderLayerMask kAllRenderLayers = BuildAllRenderLayerMask();

	/// @brief RenderLayerの表示名を取得する
	/// @param layer 表示名を取得するレイヤー
	/// @return レイヤーの表示名。未定義の場合はUnknown
	inline constexpr const char* GetRenderLayerName(RenderLayer layer) {
		if (!IsValidRenderLayer(layer)) {
			return "Unknown";
		}

		return kRenderLayerNames[static_cast<uint32_t>(layer)];
	}

	/// @brief RenderLayerMaskの表示名を取得する
	/// @param layerMask 表示名を取得するレイヤーマスク
	/// @return レイヤーマスクの表示名。未定義の組み合わせの場合はCustom
	inline constexpr const char* GetRenderLayerMaskName(RenderLayerMask layerMask) {
		if (layerMask == kAllRenderLayers) {
			return "All";
		}

		for (uint32_t index = 0; index < kRenderLayerCount; ++index) {
			const RenderLayer layer = GetRenderLayerByIndex(index);
			if (layerMask == ToRenderLayerMask(layer)) {
				return GetRenderLayerName(layer);
			}
		}

		return "Custom";
	}

	/// @brief マスクに指定レイヤーが含まれているか確認する
	/// @param mask 判定対象のマスク
	/// @param layer 判定対象のレイヤー
	/// @return 含まれている場合はtrue
	inline constexpr bool ContainsRenderLayer(RenderLayerMask mask, RenderLayer layer) {
		return (mask & ToRenderLayerMask(layer)) != 0;
	}

	/// @brief マスクから指定レイヤーを除外する
	/// @param mask 元のマスク
	/// @param layer 除外するレイヤー
	/// @return 指定レイヤーを除外したマスク
	inline constexpr RenderLayerMask RemoveRenderLayer(RenderLayerMask mask, RenderLayer layer) {
		return mask & ~ToRenderLayerMask(layer);
	}

	/// @brief マスクから指定レイヤーマスクを除外する
	/// @param mask 元のマスク
	/// @param removeMask 除外するレイヤーマスク
	/// @return 指定レイヤーマスクを除外したマスク
	inline constexpr RenderLayerMask RemoveRenderLayerMask(RenderLayerMask mask, RenderLayerMask removeMask) {
		return mask & ~removeMask;
	}

} // namespace MadoEngine::Render
