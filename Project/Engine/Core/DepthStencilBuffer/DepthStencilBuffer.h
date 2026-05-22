#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>

namespace MadoEngine::Core {
	class DxDevice;
	class DSVManager;

	/// @brief デプスステンシルバッファをカプセル化するクラス
	/// @details リソース生成・DSV作成・ハンドル取得を一元管理する
	class DepthStencilBuffer {
	public:
		DepthStencilBuffer()  = default;
		~DepthStencilBuffer() = default;

		// コピー禁止、ムーブ許可
		DepthStencilBuffer(const DepthStencilBuffer&)            = delete;
		DepthStencilBuffer& operator=(const DepthStencilBuffer&) = delete;
		DepthStencilBuffer(DepthStencilBuffer&&)                 = default;
		DepthStencilBuffer& operator=(DepthStencilBuffer&&)      = default;

		/// @brief デプスステンシルバッファを初期化する
		/// @param device DxDeviceのポインタ
		/// @param dsvManager DSVManagerのポインタ
		/// @param width バッファ幅
		/// @param height バッファ高さ
		/// @param format デプスフォーマット（デフォルト: DXGI_FORMAT_D32_FLOAT）
		void Initialize(
			DxDevice*   device,
			DSVManager* dsvManager,
			uint32_t    width,
			uint32_t    height,
			DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT
		);

		/// @brief DSVデスクリプタインデックスを取得する
		/// @return DSVデスクリプタインデックス
		uint32_t GetDSVIndex() const { return dsvIndex_; }

		/// @brief DSVのCPUデスクリプタハンドルを取得する
		/// @return CPUデスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle() const;

		/// @brief デプスバッファリソースを取得する
		/// @return ID3D12Resource ポインタ
		ID3D12Resource* GetResource() const { return textureResource_.Get(); }

		/// @brief バッファ幅を取得する
		/// @return 幅（ピクセル）
		uint32_t GetWidth()  const { return width_; }

		/// @brief バッファ高さを取得する
		/// @return 高さ（ピクセル）
		uint32_t GetHeight() const { return height_; }

		/// @brief デプスフォーマットを取得する
		/// @return DXGIフォーマット
		DXGI_FORMAT GetFormat() const { return format_; }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;

		uint32_t dsvIndex_ = 0;
		uint32_t width_    = 0;
		uint32_t height_   = 0;
		DXGI_FORMAT format_ = DXGI_FORMAT_D32_FLOAT;

		// ハンドル取得用にマネージャーの参照を保持する
		DSVManager* dsvManager_ = nullptr;
	};

} // namespace MadoEngine::Core
