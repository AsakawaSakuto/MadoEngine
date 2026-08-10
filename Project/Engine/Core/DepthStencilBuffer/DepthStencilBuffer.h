#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>

namespace MadoEngine::Core {
	class DxDevice;
	class DSVManager;
	class SRVManager;

	/// @brief デプスステンシルバッファを管理するクラス
	/// @details DSVとSRVを作成し、深度を書き込み用またはシェーダー参照用に状態遷移する。
	class DepthStencilBuffer {
	public:
		DepthStencilBuffer()  = default;
		~DepthStencilBuffer() = default;

		// コピー禁止、ムーブ許可
		DepthStencilBuffer(const DepthStencilBuffer&)            = delete;
		DepthStencilBuffer& operator=(const DepthStencilBuffer&) = delete;
		DepthStencilBuffer(DepthStencilBuffer&&)                 = default;
		DepthStencilBuffer& operator=(DepthStencilBuffer&&)      = default;

		/// @brief デプスステンシルバッファを初期化
		/// @param device DxDeviceのポインタ
		/// @param dsvManager DSVManagerのポインタ
		/// @param width バッファ幅
		/// @param height バッファ高さ
		/// @param format デプスフォーマット
		void Initialize(
			DxDevice*   device,
			DSVManager* dsvManager,
			uint32_t    width,
			uint32_t    height,
			DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT
		);

		/// @brief デプスステンシルバッファを初期化
		/// @param device DxDeviceのポインタ
		/// @param dsvManager DSVManagerのポインタ
		/// @param srvManager SRVManagerのポインタ
		/// @param width バッファ幅
		/// @param height バッファ高さ
		/// @param format デプスフォーマット
		void Initialize(
			DxDevice*   device,
			DSVManager* dsvManager,
			SRVManager* srvManager,
			uint32_t    width,
			uint32_t    height,
			DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT
		);

		/// @brief デプスステンシルバッファをリサイズ
		/// @param width 新しいバッファ幅
		/// @param height 新しいバッファ高さ
		void Resize(uint32_t width, uint32_t height);

		/// @brief DSVディスクリプタインデックスを取得
		/// @return DSVディスクリプタインデックス
		uint32_t GetDSVIndex() const { return dsvIndex_; }

		/// @brief SRVディスクリプタインデックスを取得
		/// @return SRVディスクリプタインデックス
		uint32_t GetSRVIndex() const { return srvIndex_; }

		/// @brief DSVのCPUディスクリプタハンドルを取得
		/// @return CPUディスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle() const;

		/// @brief SRVのCPUディスクリプタハンドルを取得
		/// @return CPUディスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() const;

		/// @brief SRVのGPUディスクリプタハンドルを取得
		/// @return GPUディスクリプタハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;

		/// @brief 深度リソースの状態を遷移
		/// @param commandList コマンドリスト
		/// @param stateAfter 遷移後の状態
		void Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES stateAfter);

		/// @brief デプスバッファリソースを取得
		/// @return ID3D12Resourceポインタ
		ID3D12Resource* GetResource() const { return textureResource_.Get(); }

		/// @brief バッファ幅を取得
		/// @return 幅
		uint32_t GetWidth()  const { return width_; }

		/// @brief バッファ高さを取得
		/// @return 高さ
		uint32_t GetHeight() const { return height_; }

		/// @brief デプスフォーマットを取得
		/// @return DXGIフォーマット
		DXGI_FORMAT GetFormat() const { return format_; }

	private:
		/// @brief 深度リソースとDSV/SRVを作成
		void CreateResourceAndView();

		/// @brief 深度リソース用フォーマットを取得
		/// @return DXGIフォーマット
		DXGI_FORMAT GetResourceFormat() const;

		/// @brief 深度SRV用フォーマットを取得
		/// @return DXGIフォーマット
		DXGI_FORMAT GetShaderResourceFormat() const;

		DxDevice* device_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;

		uint32_t dsvIndex_ = 0;
		uint32_t srvIndex_ = 0;
		uint32_t width_    = 0;
		uint32_t height_   = 0;
		DXGI_FORMAT format_ = DXGI_FORMAT_D32_FLOAT;
		D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

		DSVManager* dsvManager_ = nullptr;
		SRVManager* srvManager_ = nullptr;
	};

} // namespace MadoEngine::Core
