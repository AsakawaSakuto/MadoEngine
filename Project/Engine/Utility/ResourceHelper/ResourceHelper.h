#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
#include <cassert>
#include "Utility/Logger/Logger.h"

/// @brief バッファリソースを生成する
/// @param device D3D12デバイスのポインタ
/// @param sizeInBytes バッファのバイトサイズ
/// @return 生成されたバッファリソース
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

/// @brief 深度ステンシルテクスチャリソースを生成する
/// @param device D3D12デバイスのポインタ
/// @param width テクスチャの幅
/// @param height テクスチャの高さ
/// @return 生成された深度ステンシルリソース
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

/// @brief 型付きバッファリソースを生成しCPU書き込み可能なポインタにマップする
/// @tparam T バッファ要素の型
/// @param device D3D12デバイスのポインタ
/// @param outResource 生成されたリソースの出力先
/// @param count 要素数（デフォルト1）
/// @return マップされたT型ポインタ
template<typename T>
T* CreateMappedBuffer(ID3D12Device* device, Microsoft::WRL::ComPtr<ID3D12Resource>& outResource, size_t count = 1) {
	outResource = CreateBufferResource(device, sizeof(T) * count);
	T* ptr = nullptr;
	outResource->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
	return ptr;
}