#include "Render/Screen/RenderTexture.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/View/RTVManager.h"
#include "Core/View/SRVManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Render {

	void RenderTexture::Initialize(
		MadoEngine::Core::DxDevice*   device,
		MadoEngine::Core::RTVManager* rtvManager,
		MadoEngine::Core::SRVManager* srvManager,
		uint32_t                      width,
		uint32_t                      height,
		DXGI_FORMAT                   format)
	{
		assert(device     != nullptr && "DxDevice が nullptr です");
		assert(rtvManager != nullptr && "RTVManager が nullptr です");
		assert(srvManager != nullptr && "SRVManager が nullptr です");
		assert(width  > 0            && "width は 0 より大きい値を指定してください");
		assert(height > 0            && "height は 0 より大きい値を指定してください");

		device_ = device;
		rtvManager_ = rtvManager;
		srvManager_ = srvManager;
		width_      = width;
		height_     = height;
		format_     = format;
		rtvIndex_ = rtvManager_->Allocate();
		srvIndex_ = srvManager_->Allocate();

		CreateResourceAndViews();
	}

	void RenderTexture::Resize(uint32_t width, uint32_t height) {
		assert(device_ != nullptr && "DxDevice が nullptr です");
		assert(rtvManager_ != nullptr && "RTVManager が nullptr です");
		assert(srvManager_ != nullptr && "SRVManager が nullptr です");
		assert(width > 0 && height > 0);

		if (width_ == width && height_ == height) {
			return;
		}

		width_ = width;
		height_ = height;
		CreateResourceAndViews();

		Logger::Output(
			"レンダーテクスチャをリサイズしました: " +
			std::to_string(width_) + "x" + std::to_string(height_),
			Logger::Level::Engine
		);
	}

	void RenderTexture::CreateResourceAndViews() {
		// --- リソースデスクリプタの設定 ---
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment          = 0;
		desc.Width              = width_;
		desc.Height             = height_;
		desc.DepthOrArraySize   = 1;
		desc.MipLevels          = 1;
		desc.Format             = format_;
		desc.SampleDesc.Count   = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		// --- ヒーププロパティの設定（DEFAULT ヒープ）---
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask     = 1;
		heapProps.VisibleNodeMask      = 1;

		// --- クリアカラーの設定 ---
		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format   = format_;
		clearValue.Color[0] = 0.1f;
		clearValue.Color[1] = 0.25f;
		clearValue.Color[2] = 0.5f;
		clearValue.Color[3] = 1.0f;

		// --- テクスチャリソースの生成 ---
		textureResource_.Reset();
		HRESULT hr = device_->GetDevice()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(&textureResource_)
		);
		assert(SUCCEEDED(hr) && "テクスチャリソースの生成に失敗しました");

		currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		Logger::Output("テクスチャリソースを生成しました", Logger::Level::Engine);

		// --- RTV の生成 ---
		rtvManager_->CreateRenderTargetView(textureResource_.Get(), rtvIndex_, format_);

		Logger::Output("RTVを生成しました (index=" + std::to_string(rtvIndex_) + ")", Logger::Level::Engine);

		// --- SRV の生成 ---
		srvManager_->CreateShaderResourceView(textureResource_.Get(), srvIndex_, format_);

		Logger::Output("SRVを生成しました (index=" + std::to_string(srvIndex_) + ")", Logger::Level::Engine);
	}

	void RenderTexture::BeginRender(
		ID3D12GraphicsCommandList*  commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle)
	{
		assert(commandList      != nullptr && "commandList が nullptr です");
		assert(textureResource_ != nullptr && "テクスチャリソースが未初期化です");

		// PIXEL_SHADER_RESOURCE → RENDER_TARGET へのリソースバリア
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = textureResource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);

		currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

		// RTV / DSV をセット
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager_->GetCPUHandle(rtvIndex_);
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthStencilHandle);

		// レンダーターゲットのクリア
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}

	void RenderTexture::EndRender(ID3D12GraphicsCommandList* commandList) {
		assert(commandList      != nullptr && "commandList が nullptr です");
		assert(textureResource_ != nullptr && "テクスチャリソースが未初期化です");

		// RENDER_TARGET → PIXEL_SHADER_RESOURCE へのリソースバリア
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = textureResource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);

		currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetRTVCPUHandle() const {
		assert(rtvManager_ != nullptr && "RTVManager が nullptr です");
		return rtvManager_->GetCPUHandle(rtvIndex_);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetSRVCPUHandle() const {
		assert(srvManager_ != nullptr && "SRVManager が nullptr です");
		return srvManager_->GetCPUHandle(srvIndex_);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderTexture::GetSRVGPUHandle() const {
		assert(srvManager_ != nullptr && "SRVManager が nullptr です");
		return srvManager_->GetGPUHandle(srvIndex_);
	}

} // namespace MadoEngine::Render
