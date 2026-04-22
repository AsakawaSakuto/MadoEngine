#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>

namespace MadoEngine::Core {
	class DxDevice;
	class CommandManager;
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
		void Initialize(
			Core::DxDevice* device,
			Core::CommandManager* commandManager,
			HWND hwnd,
			uint32_t width,
			uint32_t height,
			uint32_t bufferCount = 2
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

	private:
		/// @brief SwapChainの生成
		void CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height);

		Core::DxDevice* device_ = nullptr;
		Core::CommandManager* commandManager_ = nullptr;

		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
		uint32_t bufferCount_ = 2;
	};
}