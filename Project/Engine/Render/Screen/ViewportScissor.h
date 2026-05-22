#pragma once
#include <d3d12.h>
#include <cstdint>
#include <cassert>

namespace MadoEngine::Render {

	/// @brief ビューポートとシザー矩形を一元管理するクラス
	/// @details D3D12_VIEWPORT と D3D12_RECT のカプセル化、
	///          コマンドリストへの適用、ウィンドウリサイズ対応を担う
	class ViewportScissor {
	public:
		ViewportScissor()  = default;
		~ViewportScissor() = default;

		// コピー禁止、ムーブ許可
		ViewportScissor(const ViewportScissor&)            = delete;
		ViewportScissor& operator=(const ViewportScissor&) = delete;
		ViewportScissor(ViewportScissor&&)                 = default;
		ViewportScissor& operator=(ViewportScissor&&)      = default;

		/// @brief ビューポートとシザー矩形をウィンドウサイズで初期化・更新する
		/// @param width  描画領域の幅（ピクセル）
		/// @param height 描画領域の高さ（ピクセル）
		void UpdateSize(uint32_t width, uint32_t height);

		/// @brief ビューポートとシザー矩形をコマンドリストに適用する
		/// @param commandList ID3D12GraphicsCommandList のポインタ
		void Apply(ID3D12GraphicsCommandList* commandList) const;

		/// @brief ビューポートを取得する
		/// @return D3D12_VIEWPORT の const 参照
		const D3D12_VIEWPORT& GetViewport() const { return viewport_; }

		/// @brief シザー矩形を取得する
		/// @return D3D12_RECT の const 参照
		const D3D12_RECT& GetScissorRect() const { return scissorRect_; }

	private:
		D3D12_VIEWPORT viewport_     = {};
		D3D12_RECT     scissorRect_  = {};
	};

} // namespace MadoEngine::Render
