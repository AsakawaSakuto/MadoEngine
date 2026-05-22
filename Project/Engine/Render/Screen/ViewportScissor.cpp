#include "Render/Screen/ViewportScissor.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Render {

	void ViewportScissor::UpdateSize(uint32_t width, uint32_t height) {
		assert(width  > 0 && "width は 0 より大きい値を指定してください");
		assert(height > 0 && "height は 0 より大きい値を指定してください");

		// --- ビューポートの設定 ---
		viewport_.TopLeftX = 0.0f;
		viewport_.TopLeftY = 0.0f;
		viewport_.Width    = static_cast<FLOAT>(width);
		viewport_.Height   = static_cast<FLOAT>(height);
		viewport_.MinDepth = 0.0f;
		viewport_.MaxDepth = 1.0f;

		// --- シザー矩形の設定 ---
		scissorRect_.left   = 0;
		scissorRect_.top    = 0;
		scissorRect_.right  = static_cast<LONG>(width);
		scissorRect_.bottom = static_cast<LONG>(height);

		Logger::Output(
			"ViewportScissor を更新しました: width=" + std::to_string(width) +
			", height=" + std::to_string(height),
			Logger::Level::Engine
		);
	}

	void ViewportScissor::Apply(ID3D12GraphicsCommandList* commandList) const {
		assert(commandList != nullptr && "commandList が nullptr です");

		commandList->RSSetViewports(1, &viewport_);
		commandList->RSSetScissorRects(1, &scissorRect_);
	}

} // namespace MadoEngine::Render
