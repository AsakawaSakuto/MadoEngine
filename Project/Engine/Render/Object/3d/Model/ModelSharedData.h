#pragma once
#include "ModelData.h"
#include "ModelType.h"
#include "../Animation/AnimationStruct.h"
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
	Animation animationData;

	std::vector<std::string> textureNames;
	std::vector<uint32_t> textureIndices;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

	MadoEngine::Render::PSODesc psoDesc;
};

namespace MadoEngine::ModelResource {

void Initialize(ModelSharedData& outData, ID3D12Device* device, const std::string& modelPath, ModelType requestedType = ModelType::Auto);
void Finalize(ModelSharedData& data);
MadoEngine::Render::PSODesc CreatePSODesc(ModelType type);
std::string ModelTypeToString(ModelType type);

}
