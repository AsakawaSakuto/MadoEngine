#include "ModelMaterialBuffer.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <cassert>
#include <cstring>

void ModelMaterialBuffer::Initialize(ID3D12Device* device, const std::vector<Vector4>& materialColors) {
	assert(device);
	materialColors_ = materialColors;
	if (materialColors_.empty()) {

		// Materialを持たないAssetでも先頭定数Bufferへの参照を保証
		materialColors_.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	// Root CBVごとの256 Byte境界を保ち、複数Materialを単一Resourceへ配置
	mappedData_ = CreateMappedBuffer<std::byte>(device, resource_, kMaterialStride * materialColors_.size());
	Update(ModelMaterial{});
}

void ModelMaterialBuffer::Update(const ModelMaterial& material) {
	assert(mappedData_);
	for (size_t materialIndex = 0; materialIndex < materialColors_.size(); ++materialIndex) {

		// Draw間で同じ領域を上書きすると全SubMeshが最後の色を参照するためMaterial別に保持
		ModelMaterial gpuMaterial = material;
		gpuMaterial.color = material.color * materialColors_[materialIndex];
		std::memcpy(mappedData_ + kMaterialStride * materialIndex, &gpuMaterial, sizeof(gpuMaterial));
	}
}

D3D12_GPU_VIRTUAL_ADDRESS ModelMaterialBuffer::GetGPUVirtualAddress(size_t materialIndex) const {
	assert(resource_);
	if (materialIndex >= materialColors_.size()) {

		// 不正なMaterial IndexでもResource範囲内の定数Bufferへ制限
		materialIndex = 0;
	}
	return resource_->GetGPUVirtualAddress() + kMaterialStride * materialIndex;
}
