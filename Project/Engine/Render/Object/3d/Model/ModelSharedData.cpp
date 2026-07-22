#include "ModelSharedData.h"
#include "Core/TextureManager/TextureManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include "../Animation/AnimationFunction.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace {

std::string ToLower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

ModelType InferModelType(const ModelData& modelData, const Animation& animationData) {
	if (!modelData.skinClusterData.empty()) {
		return ModelType::Skinning;
	}
	if (!animationData.nodeAnimations.empty()) {
		return ModelType::Animated;
	}
	return ModelType::Static;
}

} // namespace

namespace MadoEngine::ModelResource {

MadoEngine::Render::PSODesc CreatePSODesc(ModelType type) {
	MadoEngine::Render::PSODesc desc;
	desc.blendMode = MadoEngine::Render::BlendMode::Normal;
	desc.depthMode = MadoEngine::Render::DepthMode::ReadWrite;
	desc.cullMode = MadoEngine::Render::CullMode::Back;
	desc.fillMode = MadoEngine::Render::FillMode::Solid;
	desc.topology = MadoEngine::Render::TopologyType::Triangle;

	switch (type) {
	case ModelType::Skinning:
		desc.inputLayout = MadoEngine::Render::InputLayoutType::SkiningModel;
		desc.vsKey = "Object3d/Model/SkinningModel.VS";
		desc.psKey = "Object3d/Model/SkinningModel.PS";
		desc.rootSigKey = "SkinningModel.RootSig";
		break;
	case ModelType::Animated:
	case ModelType::Static:
	default:
		desc.inputLayout = MadoEngine::Render::InputLayoutType::StaticModel;
		desc.vsKey = "Object3d/Model/Model.VS";
		desc.psKey = "Object3d/Model/Model.PS";
		desc.rootSigKey = "Model.RootSig";
		break;
	}

	return desc;
}

std::string ModelTypeToString(ModelType type) {
	switch (type) {
	case ModelType::Static: return "Static";
	case ModelType::Animated: return "Animated";
	case ModelType::Skinning: return "Skinning";
	default: return "Unknown";
	}
}

void Initialize(ModelSharedData& outData, ID3D12Device* device, const std::string& modelPath, ModelType requestedType) {
	assert(device);

	std::filesystem::path path(modelPath);
	const std::string extension = ToLower(path.extension().string());

	outData = {};
	outData.name = path.stem().string();
	outData.path = path.generic_string();

	outData.modelData = LoadObject3dFile(outData.path);
	if (extension == ".gltf" || extension == ".glb") {
		outData.animationData = LoadAnimationFile(outData.path);
	}

	const ModelType inferredType = InferModelType(outData.modelData, outData.animationData);
	outData.type = (requestedType == ModelType::Auto) ? inferredType : requestedType;

	if (outData.type == ModelType::Skinning && outData.modelData.skinClusterData.empty()) {
		Logger::Output("ModelResource : Skinning data was not found, fallback to " + ModelTypeToString(inferredType) + " : " + outData.path, Logger::Level::Warning);
		outData.type = inferredType;
	}

	outData.psoDesc = CreatePSODesc(outData.type);

	outData.textureNames.resize(outData.modelData.materialPaths.size());
	outData.textureIndices.resize(outData.modelData.materialPaths.size());
	for (size_t i = 0; i < outData.modelData.materialPaths.size(); ++i) {
		outData.textureNames[i] = std::filesystem::path(outData.modelData.materialPaths[i]).stem().string();
		outData.textureIndices[i] = MadoEngine::TextureManager::GetInstance().GetTextureIndex(outData.textureNames[i]);
		if (outData.textureIndices[i] == UINT32_MAX) {
			outData.textureNames[i] = "uvChecker";
			outData.textureIndices[i] = MadoEngine::TextureManager::GetInstance().GetTextureIndex(outData.textureNames[i]);
		}
	}

	ModelVertexData* vertexData = CreateMappedBuffer<ModelVertexData>(device, outData.vertexResource, outData.modelData.vertices.size());
	std::memcpy(vertexData, outData.modelData.vertices.data(), sizeof(ModelVertexData) * outData.modelData.vertices.size());

	uint32_t* indexData = CreateMappedBuffer<uint32_t>(device, outData.indexResource, outData.modelData.indeces.size());
	std::memcpy(indexData, outData.modelData.indeces.data(), sizeof(uint32_t) * outData.modelData.indeces.size());

	outData.vertexBufferView.BufferLocation = outData.vertexResource->GetGPUVirtualAddress();
	outData.vertexBufferView.SizeInBytes = UINT(sizeof(ModelVertexData) * outData.modelData.vertices.size());
	outData.vertexBufferView.StrideInBytes = sizeof(ModelVertexData);

	outData.indexBufferView.BufferLocation = outData.indexResource->GetGPUVirtualAddress();
	outData.indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * outData.modelData.indeces.size());
	outData.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void Finalize(ModelSharedData& data) {
	data.vertexResource.Reset();
	data.indexResource.Reset();
	data.vertexBufferView = {};
	data.indexBufferView = {};
}

} // namespace MadoEngine::ModelResource
