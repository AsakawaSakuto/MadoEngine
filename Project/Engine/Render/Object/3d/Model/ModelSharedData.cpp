#include "ModelSharedData.h"
#include "ModelCache.h"
#include "Core/TextureManager/TextureManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include "Utility/AssetKey.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <utility>

namespace {

/// @brief 読み込んだDataから利用可能なModel種別を推定
/// @param modelData Model Data
/// @param animationSet Animation Set
/// @return 推定したModel種別
ModelType InferModelType(const ModelData& modelData, const AnimationSet& animationSet) {
	if (!modelData.skinClusterData.empty()) {
		return ModelType::Skinning;
	}
	if (!animationSet.IsEmpty()) {
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
		desc.inputLayout = MadoEngine::Render::InputLayoutType::SkinningModel;
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

bool Initialize(ModelSharedData& outData, ID3D12Device* device, const std::string& modelPath,
    ModelType requestedType, bool* loadedFromCache) {
	assert(device);

	std::filesystem::path path(modelPath);

	outData = {};
	outData.name = path.stem().string();
	outData.path = path.generic_string();


    // 配布済みの変換結果を優先し、欠損や更新時だけ従来のAssimp経路へ復旧
    ModelAssetData asset;
    const ModelCache cache;
    const bool usedCache = cache.Load(path, asset);
    if (!usedCache && !ModelAssetImporter::Load(path, asset)) {
        return false;
    }
    outData.modelData = std::move(asset.modelData);
    outData.animationSet = std::move(asset.animationSet);

	const ModelType inferredType = InferModelType(outData.modelData, outData.animationSet);
	if ((requestedType == ModelType::Auto)) {
		outData.type = inferredType;
	} else {
		outData.type = requestedType;
	}

	if (outData.type == ModelType::Skinning && outData.modelData.skinClusterData.empty()) {

		// Skin Clusterを持たないAssetをSkinning Pipelineへ渡さないため実データに合わせて降格
		Logger::Output("ModelResource: Skinning Dataがないため" + ModelTypeToString(inferredType) + "へ切り替え: " + outData.path, Logger::Level::Warning);
		outData.type = inferredType;
	}

	outData.psoDesc = CreatePSODesc(outData.type);

	outData.textureNames.resize(outData.modelData.materialPaths.size());
	outData.textureIndices.resize(outData.modelData.materialPaths.size());
	for (size_t i = 0; i < outData.modelData.materialPaths.size(); ++i) {
		outData.textureNames[i] = std::filesystem::path(outData.modelData.materialPaths[i]).stem().string();
		outData.textureIndices[i] = MadoEngine::TextureManager::GetInstance().GetTextureIndex(outData.textureNames[i]);
		if (outData.textureIndices[i] == UINT32_MAX) {

			// 欠損TextureでもMaterialの拡散反射色を表示できるよう白Textureへ置換
			outData.textureNames[i] = "white1x1";
			outData.textureIndices[i] = MadoEngine::TextureManager::GetInstance().GetTextureIndex(outData.textureNames[i]);
		}
	}

	// 全Instanceから共有する不変MeshをUpload Heapへ一度だけ展開
	ModelVertexData* vertexData = CreateMappedBuffer<ModelVertexData>(device, outData.vertexResource, outData.modelData.vertices.size());
	std::memcpy(vertexData, outData.modelData.vertices.data(), sizeof(ModelVertexData) * outData.modelData.vertices.size());

	uint32_t* indexData = CreateMappedBuffer<uint32_t>(device, outData.indexResource, outData.modelData.indices.size());
	std::memcpy(indexData, outData.modelData.indices.data(), sizeof(uint32_t) * outData.modelData.indices.size());

	outData.vertexBufferView.BufferLocation = outData.vertexResource->GetGPUVirtualAddress();
	outData.vertexBufferView.SizeInBytes = UINT(sizeof(ModelVertexData) * outData.modelData.vertices.size());
	outData.vertexBufferView.StrideInBytes = sizeof(ModelVertexData);

	outData.indexBufferView.BufferLocation = outData.indexResource->GetGPUVirtualAddress();
	outData.indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * outData.modelData.indices.size());
	outData.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    if (loadedFromCache) {
        *loadedFromCache = usedCache;
    }
    return true;
}

void Finalize(ModelSharedData& data) {
	data.vertexResource.Reset();
	data.indexResource.Reset();
	data.vertexBufferView = {};
	data.indexBufferView = {};
}

} // namespace MadoEngine::ModelResource
