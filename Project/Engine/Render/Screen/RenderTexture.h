#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>

namespace MadoEngine::Core {
	class DxDevice;
	class RTVManager;
	class SRVManager;
}

namespace MadoEngine::Render {

	/// @brief レンダー・ツー・テクスチャをカプセル化するクラス
	/// @details オフスクリーンレンダーターゲット（RTV/SRV）のリソース管理・ステート遷移を担う
	class RenderTexture {
	public:
		RenderTexture()  = default;
		~RenderTexture() = default;

		// コピー禁止、ムーブ許可
		RenderTexture(const RenderTexture&)            = delete;
		RenderTexture& operator=(const RenderTexture&) = delete;
		RenderTexture(RenderTexture&&)                 = default;
		RenderTexture& operator=(RenderTexture&&)      = default;

		/// @brief レンダーテクスチャを初期化する
		/// @param device DxDeviceのポインタ
		/// @param rtvManager RTVManagerのポインタ
		/// @param srvManager SRVManagerのポインタ
		/// @param width テクスチャ幅
		/// @param height テクスチャ高さ
		/// @param format テクスチャフォーマット（デフォルト: DXGI_FORMAT_R8G8B8A8_UNORM）
		void Initialize(
			MadoEngine::Core::DxDevice*    device,
			MadoEngine::Core::RTVManager*  rtvManager,
			MadoEngine::Core::SRVManager*  srvManager,
			uint32_t                       width,
			uint32_t                       height,
			DXGI_FORMAT                    format = DXGI_FORMAT_R8G8B8A8_UNORM
		);

		/// @brief 描画開始処理を行う（PSR → RTV へのリソースバリア、RTV/DSV セット、クリア）
		/// @param commandList コマンドリスト
		/// @param depthStencilHandle 深度ステンシルビューのCPUハンドル
		void BeginRender(
			ID3D12GraphicsCommandList*   commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE  depthStencilHandle
		);

		/// @brief 描画終了処理を行う（RTV → PSR へのリソースバリア）
		/// @param commandList コマンドリスト
		void EndRender(ID3D12GraphicsCommandList* commandList);

		/// @brief RTVのインデックスを取得する
		/// @return RTVデスクリプタインデックス
		uint32_t GetRTVIndex() const { return rtvIndex_; }

		/// @brief SRVのインデックスを取得する
		/// @return SRVデスクリプタインデックス
		uint32_t GetSRVIndex() const { return srvIndex_; }

		/// @brief RTVのCPUデスクリプタハンドルを取得する
		/// @return CPUデスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle() const;

		/// @brief SRVのCPUデスクリプタハンドルを取得する
		/// @return CPUデスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() const;

		/// @brief SRVのGPUデスクリプタハンドルを取得する
		/// @return GPUデスクリプタハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;

		/// @brief テクスチャリソースを取得する
		/// @return ID3D12Resource ポインタ
		ID3D12Resource* GetResource() const { return textureResource_.Get(); }

		/// @brief テクスチャ幅を取得する
		/// @return 幅（ピクセル）
		uint32_t GetWidth()  const { return width_; }

		/// @brief テクスチャ高さを取得する
		/// @return 高さ（ピクセル）
		uint32_t GetHeight() const { return height_; }

		/// @brief テクスチャフォーマットを取得する
		/// @return DXGIフォーマット
		DXGI_FORMAT GetFormat() const { return format_; }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;

		uint32_t rtvIndex_ = 0;
		uint32_t srvIndex_ = 0;

		D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		uint32_t    width_  = 0;
		uint32_t    height_ = 0;
		DXGI_FORMAT format_ = DXGI_FORMAT_R8G8B8A8_UNORM;

		// ハンドル取得用にマネージャーの参照を保持する
		MadoEngine::Core::RTVManager* rtvManager_ = nullptr;
		MadoEngine::Core::SRVManager* srvManager_ = nullptr;
	};

} // namespace MadoEngine::Render
