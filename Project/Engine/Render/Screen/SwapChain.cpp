#include "SwapChain.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/command.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Screen {

	void SwapChain::Initialize(
		Core::DxDevice* device,
		Core::CommandManager* commandManager,
		HWND hwnd,
		uint32_t width,
		uint32_t height,
		uint32_t bufferCount
	) {
		assert(device != nullptr);
		assert(commandManager != nullptr);
		assert(hwnd != nullptr);
		assert(width > 0 && height > 0);
		assert(bufferCount >= 2);

		device_ = device;
		commandManager_ = commandManager;
		bufferCount_ = bufferCount;

		CreateSwapChain(hwnd, width, height);
	}

	uint32_t SwapChain::GetCurrentBackBufferIndex() const {
		assert(swapChain_ != nullptr);
		return swapChain_->GetCurrentBackBufferIndex();
	}

	void SwapChain::CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height) {
		// SwapChainの設定
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = width;                                  // 画面の幅
		swapChainDesc.Height = height;                                // 画面の高さ
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;            // 色の形式
		swapChainDesc.SampleDesc.Count = 1;                           // サンプル数 (マルチサンプルしない)
		swapChainDesc.SampleDesc.Quality = 0;                         // サンプル品質
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // 描画のターゲットとして利用する
		swapChainDesc.BufferCount = bufferCount_;                     // バッファ数 (ダブルバッファ)
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;     // スワップ効果 (モニタに写したら、中身を破棄)
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // フルスクリーン切り替えを許可

		// SwapChainの生成
		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
		HRESULT hr = device_->GetDxgiFactory()->CreateSwapChainForHwnd(
			commandManager_->GetCommandQueue(),
			hwnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			swapChain1.GetAddressOf()
		);
		assert(SUCCEEDED(hr));

		// IDXGISwapChain4にキャスト
		hr = swapChain1.As(&swapChain_);
		assert(SUCCEEDED(hr));

		Logger::Info("SwapChainの作成に成功しました");
	}
}
