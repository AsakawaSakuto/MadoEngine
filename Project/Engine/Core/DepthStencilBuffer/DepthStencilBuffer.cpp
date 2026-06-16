#include "Core/DepthStencilBuffer/DepthStencilBuffer.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/View/DSVManager.h"
#include "Core/View/SRVManager.h"
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
		Initialize(device, dsvManager, nullptr, width, height, format);
	}

	void DepthStencilBuffer::Initialize(
		DxDevice*   device,
		DSVManager* dsvManager,
		SRVManager* srvManager,
		uint32_t    width,
		uint32_t    height,
		DXGI_FORMAT format)
	{
		assert(device != nullptr && "DxDevice が nullptr です");
		assert(dsvManager != nullptr && "DSVManager が nullptr です");
		assert(width > 0 && "width は 0 より大きい値を指定してください");
		assert(height > 0 && "height は 0 より大きい値を指定してください");

		device_ = device;
		dsvManager_ = dsvManager;
		srvManager_ = srvManager;
		width_ = width;
		height_ = height;
		format_ = format;
		dsvIndex_ = dsvManager_->Allocate();
		if (srvManager_ != nullptr) {
			srvIndex_ = srvManager_->Allocate();
		}

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
			"[Engine] デプスステンシルバッファをリサイズしました: " +
			std::to_string(width_) + "x" + std::to_string(height_),
			Logger::Level::Engine
		);
	}

	void DepthStencilBuffer::CreateResourceAndView() {
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment          = 0;
		desc.Width              = width_;
		desc.Height             = height_;
		desc.DepthOrArraySize   = 1;
		desc.MipLevels          = 1;
		desc.Format             = GetResourceFormat();
		desc.SampleDesc.Count   = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask     = 1;
		heapProps.VisibleNodeMask      = 1;

		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format               = format_;
		clearValue.DepthStencil.Depth   = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		textureResource_.Reset();
		HRESULT hr = device_->GetDevice()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&textureResource_)
		);
		assert(SUCCEEDED(hr) && "デプスステンシルバッファリソースの生成に失敗しました");
		currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

		Logger::Output(
			"[Engine] デプスステンシルバッファリソースを生成しました: " +
			std::to_string(width_) + "x" + std::to_string(height_),
			Logger::Level::Engine
		);

		dsvManager_->CreateDepthStencilView(textureResource_.Get(), dsvIndex_, format_);
		Logger::Output("[Engine] DSVを生成しました: " + std::to_string(dsvIndex_), Logger::Level::Engine);

		if (srvManager_ != nullptr) {
			srvManager_->CreateShaderResourceView(textureResource_.Get(), srvIndex_, GetShaderResourceFormat());
			Logger::Output("[Engine] 深度SRVを生成しました: " + std::to_string(srvIndex_), Logger::Level::Engine);
		}
	}

	DXGI_FORMAT DepthStencilBuffer::GetResourceFormat() const {
		switch (format_) {
			case DXGI_FORMAT_D32_FLOAT:
				return DXGI_FORMAT_R32_TYPELESS;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				return DXGI_FORMAT_R24G8_TYPELESS;
			default:
				return format_;
		}
	}

	DXGI_FORMAT DepthStencilBuffer::GetShaderResourceFormat() const {
		switch (format_) {
			case DXGI_FORMAT_D32_FLOAT:
				return DXGI_FORMAT_R32_FLOAT;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			default:
				return format_;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBuffer::GetDSVCPUHandle() const {
		assert(dsvManager_ != nullptr && "DSVManager が nullptr です");
		return dsvManager_->GetCPUHandle(dsvIndex_);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBuffer::GetSRVCPUHandle() const {
		assert(srvManager_ != nullptr && "SRVManager が nullptr です");
		return srvManager_->GetCPUHandle(srvIndex_);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DepthStencilBuffer::GetSRVGPUHandle() const {
		assert(srvManager_ != nullptr && "SRVManager が nullptr です");
		return srvManager_->GetGPUHandle(srvIndex_);
	}

	void DepthStencilBuffer::Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES stateAfter) {
		assert(commandList != nullptr && "commandList が nullptr です");
		assert(textureResource_ != nullptr && "デプスステンシルバッファが未生成です");

		if (currentState_ == stateAfter) {
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = textureResource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter = stateAfter;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);

		currentState_ = stateAfter;
	}

} // namespace MadoEngine::Core
