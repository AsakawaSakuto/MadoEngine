#include "RTVManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "ViewFunction.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Core {

	void RTVManager::Initialize(DxDevice* device, uint32_t maxDescriptors) {
		assert(device != nullptr);
		assert(maxDescriptors > 0);

		device_ = device;
		maxDescriptors_ = maxDescriptors;
		nextIndex_ = 0;

		// 割り当て状態を初期化
		allocated_.resize(maxDescriptors_, false);

		// RTV用のデスクリプタヒープを作成（シェーダーから不可視）
		descriptorHeap_ = CreateDescriptorHeap(
			device_->GetDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			maxDescriptors_,
			false
		);

		// デスクリプタのサイズを取得
		descriptorSize_ = device_->GetDevice()->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);

		Logger::Output("RTVManagerの初期化が完了しました。最大デスクリプタ数: " + std::to_string(maxDescriptors_), Logger::Level::Engine);
	}

	uint32_t RTVManager::Allocate() {
		uint32_t index;

		// 解放されたインデックスがあれば再利用
		if (!freeIndices_.empty()) {
			index = freeIndices_.front();
			freeIndices_.pop();
			Logger::Output("RTVデスクリプタを再利用しました。インデックス: " + std::to_string(index), Logger::Level::Engine);
		} else {
			// 新しいインデックスを割り当て
			assert(nextIndex_ < maxDescriptors_ && "RTVデスクリプタヒープの容量を超えました");
			index = nextIndex_++;
			Logger::Output("新しいRTVデスクリプタを割り当てました。インデックス: " + std::to_string(index), Logger::Level::Engine);
		}

		// 割り当て状態を記録
		allocated_[index] = true;
		return index;
	}

	void RTVManager::Free(uint32_t index) {
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(allocated_[index] && "既に解放済みのデスクリプタです");

		allocated_[index] = false;
		freeIndices_.push(index);
		Logger::Output("RTVデスクリプタを解放しました。インデックス: " + std::to_string(index), Logger::Level::Engine);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RTVManager::GetCPUHandle(uint32_t index) const {
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(allocated_[index] && "解放済みまたは未割り当てのデスクリプタです");
		return GetCPUDescriptorHandle(descriptorHeap_.Get(), descriptorSize_, index);
	}

	void RTVManager::CreateRenderTargetView(
		ID3D12Resource* resource,
		uint32_t index,
		DXGI_FORMAT format
	) {
		assert(resource != nullptr);
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

		// フォーマットの決定
		if (format == DXGI_FORMAT_UNKNOWN) {
			rtvDesc.Format = resourceDesc.Format;
		} else {
			rtvDesc.Format = format;
		}

		// リソースの次元に応じて設定
		if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
			if (resourceDesc.SampleDesc.Count > 1) {
				// マルチサンプル
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			} else {
				// 通常のテクスチャ
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0;
				rtvDesc.Texture2D.PlaneSlice = 0;
			}
		} else {
			assert(false && "未対応のリソース次元です");
		}

		// RTVを作成
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(index);
		device_->GetDevice()->CreateRenderTargetView(resource, &rtvDesc, cpuHandle);

		Logger::Output("Render Target Viewの作成が完了しました。インデックス: " + std::to_string(index), Logger::Level::Engine);
	}
}
