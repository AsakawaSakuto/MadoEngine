#include "SRVManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "ViewFunction.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Core {

	void SRVManager::Initialize(DxDevice* device, uint32_t maxDescriptors) {
		assert(device != nullptr);
		assert(maxDescriptors > 0);

		device_ = device;
		maxDescriptors_ = maxDescriptors;
		nextIndex_ = 0;

		// 割り当て状態を初期化
		allocated_.resize(maxDescriptors_, false);

		// SRV用のデスクリプタヒープを作成（シェーダーから可視）
		descriptorHeap_ = CreateDescriptorHeap(
			device_->GetDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			maxDescriptors_,
			true
		);

		// デスクリプタのサイズを取得
		descriptorSize_ = device_->GetDevice()->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		Logger::Output("SRVManagerの初期化が完了しました。最大デスクリプタ数: " + std::to_string(maxDescriptors_), Logger::Level::Info);
	}

	uint32_t SRVManager::Allocate() {
		uint32_t index;

		// 解放されたインデックスがあれば再利用
		if (!freeIndices_.empty()) {
			index = freeIndices_.front();
			freeIndices_.pop();
			Logger::Output("SRVデスクリプタを再利用しました。インデックス: " + std::to_string(index), Logger::Level::Info);
		} else {
			// 新しいインデックスを割り当て
			assert(nextIndex_ < maxDescriptors_ && "SRVデスクリプタヒープの容量を超えました");
			index = nextIndex_++;
			Logger::Output("新しいSRVデスクリプタを割り当てました。インデックス: " + std::to_string(index), Logger::Level::Info);
		}

		// 割り当て状態を記録
		allocated_[index] = true;
		return index;
	}

	void SRVManager::Free(uint32_t index) {
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(allocated_[index] && "既に解放済みのデスクリプタです");

		allocated_[index] = false;
		freeIndices_.push(index);
		Logger::Output("SRVデスクリプタを解放しました。インデックス: " + std::to_string(index), Logger::Level::Info);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE SRVManager::GetCPUHandle(uint32_t index) const {
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(allocated_[index] && "解放済みまたは未割り当てのデスクリプタです");
		return GetCPUDescriptorHandle(descriptorHeap_.Get(), descriptorSize_, index);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SRVManager::GetGPUHandle(uint32_t index) const {
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(allocated_[index] && "解放済みまたは未割り当てのデスクリプタです");
		return GetGPUDescriptorHandle(descriptorHeap_.Get(), descriptorSize_, index);
	}

	void SRVManager::CreateShaderResourceView(
		ID3D12Resource* resource,
		uint32_t index,
		DXGI_FORMAT format
	) {
		assert(resource != nullptr);
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

		// フォーマットの決定
		if (format == DXGI_FORMAT_UNKNOWN) {
			srvDesc.Format = resourceDesc.Format;
		} else {
			srvDesc.Format = format;
		}

		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// リソースの次元に応じて設定
		if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvDesc.Texture1D.MipLevels = resourceDesc.MipLevels;
		} else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
		} else if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = resourceDesc.MipLevels;
		} else {
			assert(false && "未対応のリソース次元です");
		}

		// SRVを作成
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(index);
		device_->GetDevice()->CreateShaderResourceView(resource, &srvDesc, cpuHandle);

		Logger::Output("Shader Resource Viewの作成が完了しました。インデックス: " + std::to_string(index), Logger::Level::Info);
	}

	void SRVManager::CreateStructuredBufferSRV(
		ID3D12Resource* resource,
		uint32_t index,
		uint32_t numElements,
		uint32_t structureByteStride
	) {
		assert(resource != nullptr);
		assert(index < nextIndex_ && "無効なデスクリプタインデックスです");
		assert(numElements > 0);
		assert(structureByteStride > 0);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		srvDesc.Buffer.StructureByteStride = structureByteStride;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// SRVを作成
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(index);
		device_->GetDevice()->CreateShaderResourceView(resource, &srvDesc, cpuHandle);

		Logger::Output("構造化バッファSRVの作成が完了しました。インデックス: " + std::to_string(index), Logger::Level::Info);
	}
}
