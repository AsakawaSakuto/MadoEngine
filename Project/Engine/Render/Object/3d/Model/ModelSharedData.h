#pragma once
#include "ModelData.h"
#include "ModelType.h"
#include "../Animation/AnimationSet.h"
#include "Render/PSO/PSODesc.h"
#include <cstdint>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl/client.h>

struct ModelSharedData {
	std::string name;
	std::string path;
	ModelType type = ModelType::Static;

	ModelData modelData;
	AnimationSet animationSet;

	std::vector<std::string> textureNames;
	std::vector<uint32_t> textureIndices;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

	MadoEngine::Render::PSODesc psoDesc;
};

namespace MadoEngine::ModelResource {

/// @brief キャッシュを優先してModelの共有リソースを初期化
/// @param outData 初期化結果の出力先
/// @param device GPUリソースの生成先
/// @param modelPath 元Modelのパス
/// @param requestedType 要求するModel種別
/// @param loadedFromCache キャッシュを使用した場合はtrueを格納する出力先、不要の場合はnullptr
/// @return 初期化に成功した場合はtrue
bool Initialize(ModelSharedData& outData, ID3D12Device* device, const std::string& modelPath,
    ModelType requestedType = ModelType::Auto, bool* loadedFromCache = nullptr);
void Finalize(ModelSharedData& data);
MadoEngine::Render::PSODesc CreatePSODesc(ModelType type);
std::string ModelTypeToString(ModelType type);

}
