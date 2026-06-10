#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

namespace MadoEngine::Core {
	class DxDevice;
	class CommandManager;
	class RTVManager;
}

namespace MadoEngine::Screen {

	/// @brief SwapChainを管理するクラス
	class SwapChain {
	public:
		/// @brief SwapChainを初期化する
		/// @param device DxDeviceのポインタ
		/// @param commandManager CommandManagerのポインタ
		/// @param hwnd ウィンドウハンドル
		/// @param width ウィンドウの幅
		/// @param height ウィンドウの高さ
		/// @param bufferCount バッファ数（デフォルト: 2）
		/// @param rtvManager RTVManagerのポインタ
		void Initialize(
			Core::DxDevice* device,
			Core::CommandManager* commandManager,
			HWND hwnd,
			uint32_t width,
			uint32_t height,
			uint32_t bufferCount = 2,
			Core::RTVManager* rtvManager = nullptr
		);

		/// @brief バックバッファのインデックスを取得
		/// @return 現在のバックバッファインデックス
		uint32_t GetCurrentBackBufferIndex() const;

		/// @brief SwapChainを取得
		/// @return IDXGISwapChain4のポインタ
		IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }

		/// @brief バッファ数を取得
		/// @return バッファ数
		uint32_t GetBufferCount() const { return bufferCount_; }

		/// @brief 指定されたバックバッファリソースを取得
		/// @param index バッファインデックス
		/// @return バックバッファリソースのポインタ
		ID3D12Resource* GetBackBuffer(uint32_t index) const;

		/// @brief 描画開始処理（リソースバリア遷移・RTV設定・クリア）
		/// @param commandList コマンドリストのポインタ
		/// @param dsvHandle 深度ステンシルビューのCPUハンドルポインタ（不要な場合は nullptr）
		/// @param clearColor クリアカラー（不要な場合は nullptr）
		void BeginRender(ID3D12GraphicsCommandList* commandList, const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle, const float clearColor[4]);

		/// @brief 描画終了処理（リソースバリア遷移）
		/// @param commandList コマンドリストのポインタ
		void EndRender(ID3D12GraphicsCommandList* commandList);

		/// @brief 画面をスワップする（Present）
		void Present();

		/// @brief スワップチェーンのバックバッファをリサイズする
		/// @param width 新しいバックバッファ幅
		/// @param height 新しいバックバッファ高さ
		void Resize(uint32_t width, uint32_t height);

		/// @brief バックバッファ幅を取得する
		/// @return バックバッファ幅
		uint32_t GetWidth() const { return width_; }

		/// @brief バックバッファ高さを取得する
		/// @return バックバッファ高さ
		uint32_t GetHeight() const { return height_; }

		private:
		/// @brief SwapChainの生成
		void CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height);

		/// @brief バックバッファとRTVを取得・再作成する
		void CreateBackBufferViews();

		Core::DxDevice* device_ = nullptr;
		Core::CommandManager* commandManager_ = nullptr;
		Core::RTVManager* rtvManager_ = nullptr;

		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers_;
		std::vector<uint32_t> backBufferRTVIndices_;
		uint32_t bufferCount_ = 2;
		uint32_t width_ = 0;
		uint32_t height_ = 0;
	};
}
