#pragma once

#include "Render/Object/RenderLayer.h"

namespace MadoEngine::Render {

/// @brief 2D描画の連続レイヤーバッチ前後へ描画処理を挿入するインターフェース
class IRenderLayerBatchContext {
public:
	/// @brief デストラクタ
	virtual ~IRenderLayerBatchContext() = default;

	/// @brief 指定レイヤーの連続描画バッチを開始
	/// @param layer 描画するレイヤー
	virtual void BeginRenderLayerBatch(RenderLayer layer) = 0;

	/// @brief 指定レイヤーの連続描画バッチを終了
	/// @param layer 描画したレイヤー
	virtual void EndRenderLayerBatch(RenderLayer layer) = 0;
};

} // namespace MadoEngine::Render
