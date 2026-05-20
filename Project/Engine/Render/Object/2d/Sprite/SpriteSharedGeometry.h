#pragma once
#include "SpriteData.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <d3d12.h>
#include <wrl/client.h>

/// @brief 全Spriteインスタンスで共有するジオメトリバッファ（VB・IB）
/// @note このクラスの所有者は SpriteManager の唯一のインスタンスのみ
struct SpriteSharedGeometry {

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	D3D12_INDEX_BUFFER_VIEW  ibv = {};

	/// @brief 共有ジオメトリバッファを初期化する
	/// @param device D3D12デバイス
	void Initialize(ID3D12Device* device) {
		// ユニットクワッド（0〜1）で固定。size・anchorPoint はSprite側の行列で反映する
		SpriteVertexData* vertexData = CreateMappedBuffer<SpriteVertexData>(device, vertexResource, 4);
		vertexData[0].position = { 0.0f, 1.0f, 0.0f, 1.0f }; // 左下
		vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
		vertexData[2].position = { 1.0f, 1.0f, 0.0f, 1.0f }; // 右下
		vertexData[3].position = { 1.0f, 0.0f, 0.0f, 1.0f }; // 右上
		vertexData[0].texcoord = { 0.0f, 1.0f };
		vertexData[1].texcoord = { 0.0f, 0.0f };
		vertexData[2].texcoord = { 1.0f, 1.0f };
		vertexData[3].texcoord = { 1.0f, 0.0f };

		vbv.BufferLocation = vertexResource->GetGPUVirtualAddress();
		vbv.SizeInBytes    = sizeof(SpriteVertexData) * 4;
		vbv.StrideInBytes  = sizeof(SpriteVertexData);

		uint32_t* indexData = CreateMappedBuffer<uint32_t>(device, indexResource, 6);
		indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
		indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;

		ibv.BufferLocation = indexResource->GetGPUVirtualAddress();
		ibv.SizeInBytes    = sizeof(uint32_t) * 6;
		ibv.Format         = DXGI_FORMAT_R32_UINT;
	}

	/// @brief 共有ジオメトリバッファを解放する
	void Finalize() {
		vertexResource.Reset();
		indexResource.Reset();
	}
};
