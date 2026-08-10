#pragma once
#include <cstdint>
#include <string>

namespace MadoEngine::Render {
	// 描画レイヤーを追加するときは、この一覧に1行追加する
#define MADO_RENDER_LAYER_DEFINITIONS(X) \
	X(Default, "Default") \
	X(World, "World") \
	X(MapEventObject, "MapEventObject") \
	X(MapEventObjectOutline, "MapEventObjectOutline") \
	X(Player, "Player") \
	X(Effect, "Effect") \
	X(UI, "UI") \
	X(Debug, "Debug") \
    /*X(Test, "Test") \*/

	/// @brief 描画対象を分類するレイヤー
	enum class RenderLayer : uint32_t {
#define MADO_DEFINE_RENDER_LAYER_ENUM(name, serializedName) name,
		MADO_RENDER_LAYER_DEFINITIONS(MADO_DEFINE_RENDER_LAYER_ENUM)
#undef MADO_DEFINE_RENDER_LAYER_ENUM
		Count,
	};

	inline constexpr const char* kRenderLayerNames[] = {
#define MADO_DEFINE_RENDER_LAYER_NAME(name, serializedName) serializedName,
		MADO_RENDER_LAYER_DEFINITIONS(MADO_DEFINE_RENDER_LAYER_NAME)
#undef MADO_DEFINE_RENDER_LAYER_NAME
	};

#undef MADO_RENDER_LAYER_DEFINITIONS

	using RenderLayerMask = uint32_t;

	inline constexpr uint32_t kRenderLayerCount = static_cast<uint32_t>(RenderLayer::Count);

	static_assert(kRenderLayerCount == sizeof(kRenderLayerNames) / sizeof(kRenderLayerNames[0]));
	static_assert(kRenderLayerCount <= sizeof(RenderLayerMask) * 8u, "RenderLayerMaskの上限を超えています");

	/// @brief インデックスからRenderLayerを取得
	/// @param index 取得するRenderLayerのインデックス
	/// @return インデックスに対応するRenderLayer
	inline constexpr RenderLayer GetRenderLayerByIndex(uint32_t index) {
		return static_cast<RenderLayer>(index);
	}

	/// @brief RenderLayerが有効な実レイヤーか確認
	/// @param layer 確認するレイヤー
	/// @return 実レイヤーの場合はtrue
	inline constexpr bool IsValidRenderLayer(RenderLayer layer) {
		return static_cast<uint32_t>(layer) < kRenderLayerCount;
	}

	/// @brief RenderLayerをマスク値へ変換
	/// @param layer 変換対象のレイヤー
	/// @return 指定レイヤーのみを含むマスク
	inline constexpr RenderLayerMask ToRenderLayerMask(RenderLayer layer) {
		if (!IsValidRenderLayer(layer)) {
			return 0;
		}

		return 1u << static_cast<uint32_t>(layer);
	}

	/// @brief 定義済みRenderLayerをすべて含むマスクを作成
	/// @return 定義済みRenderLayerをすべて含むマスク
	inline constexpr RenderLayerMask BuildAllRenderLayerMask() {
		RenderLayerMask mask = 0;
		for (uint32_t index = 0; index < kRenderLayerCount; ++index) {
			mask |= ToRenderLayerMask(GetRenderLayerByIndex(index));
		}

		return mask;
	}

	inline constexpr RenderLayerMask kAllRenderLayers = BuildAllRenderLayerMask();

	/// @brief RenderLayerの表示名を取得
	/// @param layer 表示名を取得するレイヤー
	/// @return レイヤーの表示名、未定義の場合はUnknown
	inline constexpr const char* GetRenderLayerName(RenderLayer layer) {
		if (!IsValidRenderLayer(layer)) {
			return "Unknown";
		}

		return kRenderLayerNames[static_cast<uint32_t>(layer)];
	}

	/// @brief RenderLayerをシリアライズ用文字列へ変換
	/// @param layer 変換対象のレイヤー
	/// @return レイヤーのシリアライズ用文字列、未定義の場合はDefault
	inline std::string RenderLayerToString(RenderLayer layer) {
		if (!IsValidRenderLayer(layer)) {
			return kRenderLayerNames[static_cast<uint32_t>(RenderLayer::Default)];
		}

		return GetRenderLayerName(layer);
	}

	/// @brief シリアライズ用文字列からRenderLayerへ変換
	/// @param value 変換対象の文字列
	/// @return 文字列に対応するレイヤー、未定義の場合はDefault
	inline RenderLayer RenderLayerFromString(const std::string& value) {
		for (uint32_t index = 0; index < kRenderLayerCount; ++index) {
			if (value == kRenderLayerNames[index]) {
				return GetRenderLayerByIndex(index);
			}
		}

		return RenderLayer::Default;
	}

	/// @brief RenderLayerMaskの表示名を取得
	/// @param layerMask 表示名を取得するレイヤーマスク
	/// @return レイヤーマスクの表示名、未定義の組み合わせの場合はCustom
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

	/// @brief マスクに指定レイヤーが含まれているか確認
	/// @param mask 判定対象のマスク
	/// @param layer 判定対象のレイヤー
	/// @return 含まれている場合はtrue
	inline constexpr bool ContainsRenderLayer(RenderLayerMask mask, RenderLayer layer) {
		return (mask & ToRenderLayerMask(layer)) != 0;
	}

	/// @brief マスクから指定レイヤーを除外
	/// @param mask 元のマスク
	/// @param layer 除外するレイヤー
	/// @return 指定レイヤーを除外したマスク
	inline constexpr RenderLayerMask RemoveRenderLayer(RenderLayerMask mask, RenderLayer layer) {
		return mask & ~ToRenderLayerMask(layer);
	}

	/// @brief マスクから指定レイヤーマスクを除外
	/// @param mask 元のマスク
	/// @param removeMask 除外するレイヤーマスク
	/// @return 指定レイヤーマスクを除外したマスク
	inline constexpr RenderLayerMask RemoveRenderLayerMask(RenderLayerMask mask, RenderLayerMask removeMask) {
		return mask & ~removeMask;
	}

} // namespace MadoEngine::Render
