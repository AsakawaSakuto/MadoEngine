#include "SwapChain.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/command.h"
#include "Core/View/RTVManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Screen {

	void SwapChain::Initialize(
		Core::DxDevice* device,
		Core::CommandManager* commandManager,
		HWND hwnd,
		uint32_t width,
		uint32_t height,
		uint32_t bufferCount,
		Core::RTVManager* rtvManager
	) {
		assert(device != nullptr);
		assert(commandManager != nullptr);
		assert(hwnd != nullptr);
		assert(width > 0 && height > 0);
		assert(bufferCount >= 2);

		device_ = device;
		commandManager_ = commandManager;
		bufferCount_ = bufferCount;
		rtvManager_ = rtvManager;

		CreateSwapChain(hwnd, width, height);

		// バックバッファリソースを取得
		backBuffers_.resize(bufferCount_);
		for (uint32_t i = 0; i < bufferCount_; ++i) {
			HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
			assert(SUCCEEDED(hr));
			Logger::Output("バックバッファ " + std::to_string(i) + " を取得しました", Logger::Level::Engine);
		}

		// RTVの確保と生成
		if (rtvManager_ != nullptr) {
			backBufferRTVIndices_.resize(bufferCount_);
			for (uint32_t i = 0; i < bufferCount_; ++i) {
				backBufferRTVIndices_[i] = rtvManager_->Allocate();
				rtvManager_->CreateRenderTargetView(backBuffers_[i].Get(), backBufferRTVIndices_[i]);
			}
			Logger::Output("バックバッファ用RTVの生成が完了しました", Logger::Level::Engine);
		}
	}

	uint32_t SwapChain::GetCurrentBackBufferIndex() const {
		assert(swapChain_ != nullptr);
		return swapChain_->GetCurrentBackBufferIndex();
	}

	ID3D12Resource* SwapChain::GetBackBuffer(uint32_t index) const {
		assert(index < bufferCount_);
		return backBuffers_[index].Get();
	}

	void SwapChain::BeginRender(ID3D12GraphicsCommandList* commandList, const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle, const float clearColor[4]) {
		assert(commandList != nullptr);
		assert(rtvManager_ != nullptr);

		uint32_t index = GetCurrentBackBufferIndex();

		// PRESENT → RENDER_TARGET へ遷移
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = backBuffers_[index].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager_->GetCPUHandle(backBufferRTVIndices_[index]);
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, dsvHandle);

		if (clearColor != nullptr) {
			commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		}
	}

	void SwapChain::EndRender(ID3D12GraphicsCommandList* commandList) {
		assert(commandList != nullptr);

		uint32_t index = GetCurrentBackBufferIndex();

		// RENDER_TARGET → PRESENT へ遷移
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = backBuffers_[index].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);
	}

	void SwapChain::Present() {
		// 画面のスワップを行う（垂直同期を待つ）
		HRESULT hr = swapChain_->Present(1, 0);
		assert(SUCCEEDED(hr));
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

		Logger::Output("SwapChainの作成に成功しました", Logger::Level::Engine);
	}
}
