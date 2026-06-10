#include "Core/DepthStencilBuffer/DepthStencilBuffer.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/View/DSVManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Core {

	void DepthStencilBuffer::Initialize(
		DxDevice*   device,
		DSVManager* dsvManager,
		uint32_t    width,
		uint32_t    height,
		DXGI_FORMAT format)
	{
		assert(device     != nullptr && "DxDevice が nullptr です");
		assert(dsvManager != nullptr && "DSVManager が nullptr です");
		assert(width  > 0            && "width は 0 より大きい値を指定してください");
		assert(height > 0            && "height は 0 より大きい値を指定してください");

		device_ = device;
		dsvManager_ = dsvManager;
		width_      = width;
		height_     = height;
		format_     = format;
		dsvIndex_ = dsvManager_->Allocate();

		CreateResourceAndView();
	}

	void DepthStencilBuffer::Resize(uint32_t width, uint32_t height) {
		assert(device_ != nullptr && "DxDevice が nullptr です");
		assert(dsvManager_ != nullptr && "DSVManager が nullptr です");
		assert(width > 0 && height > 0);

		if (width_ == width && height_ == height) {
			return;
		}

		width_ = width;
		height_ = height;
		CreateResourceAndView();

		Logger::Output(
			"デプスステンシルバッファをリサイズしました: " +
			std::to_string(width_) + "x" + std::to_string(height_),
			Logger::Level::Engine
		);
	}

	void DepthStencilBuffer::CreateResourceAndView() {
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
		desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		// --- ヒーププロパティの設定（DEFAULT ヒープ）---
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask     = 1;
		heapProps.VisibleNodeMask      = 1;

		// --- クリア値の設定（Depth=1.0f, Stencil=0）---
		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format               = format_;
		clearValue.DepthStencil.Depth   = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		// --- デプスバッファリソースの生成 ---
		textureResource_.Reset();
		HRESULT hr = device_->GetDevice()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&textureResource_)
		);
		assert(SUCCEEDED(hr) && "デプスバッファリソースの生成に失敗しました");

		Logger::Output(
			"デプスバッファリソースを生成しました (size=" +
			std::to_string(width_) + "x" + std::to_string(height_) + ")",
			Logger::Level::Engine
		);

		// --- DSV の生成 ---
		dsvManager_->CreateDepthStencilView(textureResource_.Get(), dsvIndex_, format_);

		Logger::Output(
			"DSVを生成しました (index=" + std::to_string(dsvIndex_) + ")",
			Logger::Level::Engine
		);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBuffer::GetDSVCPUHandle() const {
		assert(dsvManager_ != nullptr && "DSVManager が nullptr です");
		return dsvManager_->GetCPUHandle(dsvIndex_);
	}

} // namespace MadoEngine::Core
