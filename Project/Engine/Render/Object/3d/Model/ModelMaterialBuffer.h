#pragma once
#include "ModelData.h"
#include <cstddef>
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>

/// @brief Materialごとの色とGPU定数Bufferの管理
class ModelMaterialBuffer {
public:

	/// @brief Material数に応じたGPU定数Bufferを初期化
	/// @param device D3D12 Device
	/// @param materialColors Materialごとの拡散反射色
	void Initialize(ID3D12Device* device, const std::vector<Vector4>& materialColors);

	/// @brief 共通Material設定とAssetの色をGPU定数Bufferへ反映
	/// @param material Model全体に適用するMaterial設定
	void Update(const ModelMaterial& material);

	/// @brief 指定MaterialのGPU仮想Addressを取得
	/// @param materialIndex Material Index、範囲外の場合は先頭Material
	/// @return 定数BufferのGPU仮想Address
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(size_t materialIndex = 0) const;

private:
	static constexpr size_t kMaterialStride =
		(sizeof(ModelMaterial) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) /
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

	std::vector<Vector4> materialColors_;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	std::byte* mappedData_ = nullptr;
};
