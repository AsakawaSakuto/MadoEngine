#include "DSVManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "ViewFunction.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Core {

	DSVManager& DSVManager::GetInstance() {
		static DSVManager instance;
		return instance;
	}

	void DSVManager::Initialize(DxDevice* device, uint32_t maxDescriptors) {
		assert(device != nullptr);
		assert(maxDescriptors > 0);

		device_ = device;
		maxDescriptors_ = maxDescriptors;
		nextIndex_ = 0;

		// 割り当て状態を初期化
		allocated_.resize(maxDescriptors_, false);

		// DSV用のデスクリプタヒープを作成（シェーダーから不可視）
		descriptorHeap_ = CreateDescriptorHeap(
			device_->GetDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			maxDescriptors_,
			false
		);

		// デスクリプタのサイズを取得
		descriptorSize_ = device_->GetDevice()->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV
		);

		Logger::Output("DSVManagerの初期化が完了しました。最大デスクリプタ数: " + std::to_string(maxDescriptors_), Logger::Level::Engine);
	}

	uint32_t DSVManager::Allocate() {
		uint32_t index;

		// 解放されたインデックスがあれば再利用
		if (!freeIndices_.empty()) {
			index = freeIndices_.front();
			freeIndices_.pop();
			Logger::Output("DSVデスクリプタを再利用しました。インデックス: " + std::to_string(index), Logger::Level::Engine);
		} else {
			// 新しいインデックスを割り当て
			assert(nextIndex_ < maxDescriptors_ && "DSVデスクリプタヒープの容量を超えました");
			index = nextIndex_++;
			Logger::Output("新しいDSVデスクリプタを割り当てました。インデックス: " + std::to_string(index), Logger::Level::Engine);
		}

		// 割り当て状態を記録
		allocated_[index] = true;
		return index;
	}

	void DSVManager::Free(uint32_t index) {
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(allocated_[index] && "既に解放済みのデスクリプタです");

		allocated_[index] = false;
		freeIndices_.push(index);
		Logger::Output("DSVデスクリプタを解放しました。インデックス: " + std::to_string(index), Logger::Level::Engine);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DSVManager::GetCPUHandle(uint32_t index) const {
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(allocated_[index] && "解放済みまたは未割り当てのデスクリプタです");
		return GetCPUDescriptorHandle(descriptorHeap_.Get(), descriptorSize_, index);
	}

	void DSVManager::CreateDepthStencilView(
		ID3D12Resource* resource,
		uint32_t index,
		DXGI_FORMAT format
	) {
		assert(resource != nullptr);
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

		// フォーマットの決定
		if (format == DXGI_FORMAT_UNKNOWN) {
			// リソースフォーマットからDSVフォーマットに変換
			if (resourceDesc.Format == DXGI_FORMAT_R24G8_TYPELESS) {
				dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			} else if (resourceDesc.Format == DXGI_FORMAT_R32_TYPELESS) {
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			} else if (resourceDesc.Format == DXGI_FORMAT_R16_TYPELESS) {
				dsvDesc.Format = DXGI_FORMAT_D16_UNORM;
			} else if (resourceDesc.Format == DXGI_FORMAT_R32G8X24_TYPELESS) {
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			} else {
				dsvDesc.Format = resourceDesc.Format;
			}
		} else {
			dsvDesc.Format = format;
		}

		// リソースの次元に応じて設定
		if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
			if (resourceDesc.SampleDesc.Count > 1) {
				// マルチサンプル
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			} else {
				// 通常のテクスチャ
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;
			}
		} else {
			assert(false && "未対応のリソース次元です");
		}

		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		// DSVを作成
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(index);
		device_->GetDevice()->CreateDepthStencilView(resource, &dsvDesc, cpuHandle);

		Logger::Output("Depth Stencil Viewの作成が完了しました。インデックス: " + std::to_string(index), Logger::Level::Engine);
	}
}
