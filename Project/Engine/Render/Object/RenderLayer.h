#pragma once
#include <cstdint>

namespace MadoEngine::Render {

	/// @brief 描画対象を分類するLayer
	enum class RenderLayer : uint32_t {
		Default = 1u << 0, // デフォルトレイヤー。特に指定がない場合はこちらに属する
		World = 1u << 1,   // ワールドレイヤー。主にゲーム内のオブジェクトに使用
		Player = 1u << 2,  // プレイヤーレイヤー。プレイヤーキャラクターに使用
		Effect = 1u << 3,  // エフェクトレイヤー。パーティクルや特殊効果に使用
		UI = 1u << 4,      // UIレイヤー。ユーザーインターフェースに使用
		Debug = 1u << 5    // デバッグ用レイヤー。主に開発中の情報表示に使用
	};

	using RenderLayerMask = uint32_t;

	inline constexpr RenderLayerMask kAllRenderLayers =
		static_cast<RenderLayerMask>(RenderLayer::Default) |
		static_cast<RenderLayerMask>(RenderLayer::World) |
		static_cast<RenderLayerMask>(RenderLayer::Player) |
		static_cast<RenderLayerMask>(RenderLayer::Effect) |
		static_cast<RenderLayerMask>(RenderLayer::UI) |
		static_cast<RenderLayerMask>(RenderLayer::Debug);

	/// @brief RenderLayerをマスク値へ変換する
	/// @param layer 変換対象のLayer
	/// @return 指定Layerのみを含むマスク
	inline constexpr RenderLayerMask ToRenderLayerMask(RenderLayer layer) {
		return static_cast<RenderLayerMask>(layer);
	}

	/// @brief マスクに指定Layerが含まれているか確認する
	/// @param mask 判定対象のマスク
	/// @param layer 判定対象のLayer
	/// @return 含まれている場合はtrue
	inline constexpr bool ContainsRenderLayer(RenderLayerMask mask, RenderLayer layer) {
		return (mask & ToRenderLayerMask(layer)) != 0;
	}

	/// @brief マスクから指定Layerを除外する
	/// @param mask 元のマスク
	/// @param layer 除外するLayer
	/// @return 指定Layerを除外したマスク
	inline constexpr RenderLayerMask RemoveRenderLayer(RenderLayerMask mask, RenderLayer layer) {
		return mask & ~ToRenderLayerMask(layer);
	}

	/// @brief マスクから指定Layerマスクを除外する
	/// @param mask 元のマスク
	/// @param removeMask 除外するLayerマスク
	/// @return 指定Layerマスクを除外したマスク
	inline constexpr RenderLayerMask RemoveRenderLayerMask(RenderLayerMask mask, RenderLayerMask removeMask) {
		return mask & ~removeMask;
	}

} // namespace MadoEngine::Render
