#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
#include <cassert>
#include "Utility/Logger/Logger.h"

/// @brief デスクリプタヒープを生成する関数
/// @param device D3D12デバイスのポインタ
/// @param heapType デスクリプタヒープのタイプ
/// @param numDescriptors デスクリプタの数
/// @param shaderVisible シェーダーから見えるかどうか
/// @return 生成されたデスクリプタヒープのComPtr
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

/// @brief CPUデスクリプタハンドルを取得する関数
/// @param descriptorHeap デスクリプタヒープのポインタ
/// @param descriptorSize デスクリプタのサイズ
/// @param index デスクリプタのインデックス
/// @return 取得されたCPUデスクリプタハンドル
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

/// @brief GPUデスクリプタハンドルを取得する関数
/// @param descriptorHeap デスクリプタヒープのポインタ
/// @param descriptorSize デスクリプタのサイズ
/// @param index デスクリプタのインデックス
/// @return 取得されたGPUデスクリプタハンドル
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);