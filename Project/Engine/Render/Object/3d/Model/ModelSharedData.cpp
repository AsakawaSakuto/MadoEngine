#include "ModelSharedData.h"
#include "Core/TextureManager/TextureManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include "../Animation/AnimationFunction.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <utility>

namespace {

/// @brief 文字列を小文字へ変換
/// @param value 変換対象
/// @return 小文字へ変換した文字列
std::string ToLower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

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

/// @brief ModelNode階層のNode名を収集
/// @param node 収集対象のModelNode
/// @param outNodeNames 収集したNode名の出力先
void CollectNodeNames(const ModelNode& node, std::unordered_set<std::string>& outNodeNames) {
	outNodeNames.insert(node.name);
	for (const ModelNode& child : node.children) {
		CollectNodeNames(child, outNodeNames);
	}
}

/// @brief AnimationClipとModelNode階層の互換性を検証
/// @param rootNode 互換性の基準になるRootNode
/// @param clip 検証するAnimationClip
/// @param outUnknownNodeCount Modelに存在しないAnimation Node数の出力先
/// @return すべてのAnimation NodeがModelと一致する場合はtrue
bool ValidateAnimationClip(
	const ModelNode& rootNode,
	const AnimationClip& clip,
	std::size_t& outUnknownNodeCount) {
	std::unordered_set<std::string> nodeNames;
	CollectNodeNames(rootNode, nodeNames);
	outUnknownNodeCount = 0;
	std::size_t matchedNodeCount = 0;
	for (const auto& [nodeName, nodeAnimation] : clip.nodeAnimations) {
		(void)nodeAnimation;
		if (nodeNames.contains(nodeName)) {
			++matchedNodeCount;
		} else {
			++outUnknownNodeCount;
		}
	}

	return matchedNodeCount > 0 && outUnknownNodeCount == 0;
}

/// @brief AnimationClipの再生設定をJsonから反映
/// @param json 再生設定を保持するJson
/// @param clip 設定対象のAnimationClip
void ApplyAnimationClipSettings(const nlohmann::json& json, AnimationClip& clip) {
	clip.loop = json.value("loop", true);
	const float playbackSpeed = json.value("playbackSpeed", 1.0f);
	clip.playbackSpeed = std::isfinite(playbackSpeed) ? (std::max)(0.0f, playbackSpeed) : 1.0f;
	const float blendDuration = json.value("blendDuration", 0.15f);
	clip.blendDuration = std::isfinite(blendDuration) ? (std::max)(0.0f, blendDuration) : 0.15f;
}

/// @brief Modelに対応するAnimationSetを読み込み
/// @param modelPath Modelファイルパス
/// @param rootNode 互換性検証の基準になるRootNode
/// @param outAnimationSet 読み込んだAnimationSetの出力先
void LoadAnimationSet(
	const std::filesystem::path& modelPath,
	const ModelNode& rootNode,
	AnimationSet& outAnimationSet) {
	const std::filesystem::path manifestPath =
		modelPath.parent_path() / (modelPath.stem().string() + ".animations.json");
	if (!std::filesystem::exists(manifestPath)) {
		AnimationClip clip = LoadAnimationFile(modelPath.generic_string());
		if (!clip.nodeAnimations.empty()) {
			const std::string clipName = modelPath.stem().string();
			outAnimationSet.AddClip(clipName, std::move(clip));
			outAnimationSet.SetDefaultClip(clipName);
		}
		return;
	}

	std::ifstream manifestFile(manifestPath);
	if (!manifestFile.is_open()) {
		Logger::Output("Animation設定ファイルを開けませんでした: " + manifestPath.generic_string(), Logger::Level::Warning);
		return;
	}

	try {
		nlohmann::json manifestJson;
		manifestFile >> manifestJson;
		const auto clipsIterator = manifestJson.find("clips");
		if (clipsIterator == manifestJson.end() || !clipsIterator->is_object()) {
			Logger::Output("Animation設定にclipsが存在しません: " + manifestPath.generic_string(), Logger::Level::Warning);
			return;
		}

		// Clip名はAsset内部名ではなくManifestの用途名を採用
		for (const auto& [clipName, clipJson] : clipsIterator->items()) {
			if (!clipJson.is_object()) {
				continue;
			}

			const std::string sourceFile = clipJson.value("file", modelPath.filename().generic_string());
			const std::filesystem::path sourcePath = (manifestPath.parent_path() / sourceFile).lexically_normal();
			const int animationIndex = clipJson.value("index", 0);
			AnimationClip clip = LoadAnimationFile(sourcePath.generic_string(), animationIndex);
			if (clip.nodeAnimations.empty()) {
				Logger::Output("AnimationClipを読み込めませんでした: " + clipName + " / " + sourcePath.generic_string(), Logger::Level::Warning);
				continue;
			}

			std::size_t unknownNodeCount = 0;
			if (!ValidateAnimationClip(rootNode, clip, unknownNodeCount)) {
				Logger::Output(
					"Skeletonと互換性のないAnimationClipを除外しました: " + clipName + " 不一致Node数: " + std::to_string(unknownNodeCount),
					Logger::Level::Warning
				);
				continue;
			}

			ApplyAnimationClipSettings(clipJson, clip);
			outAnimationSet.AddClip(clipName, std::move(clip));
		}

		const std::string defaultClipName = manifestJson.value("defaultClip", std::string{});
		if (!defaultClipName.empty() && !outAnimationSet.SetDefaultClip(defaultClipName)) {
			Logger::Output("標準AnimationClipが見つかりません: " + defaultClipName, Logger::Level::Warning);
		}
	} catch (const nlohmann::json::exception& exception) {
		Logger::Output(
			"Animation設定の解析に失敗しました: " + manifestPath.generic_string() + " / " + exception.what(),
			Logger::Level::Error
		);
	}
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
		LoadAnimationSet(path, outData.modelData.rootNode, outData.animationSet);
	}

	const ModelType inferredType = InferModelType(outData.modelData, outData.animationSet);
	outData.type = (requestedType == ModelType::Auto) ? inferredType : requestedType;

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

			// 欠損TextureでもDescriptor参照を有効に保つためChecker Textureへ置換
			outData.textureNames[i] = "uvChecker";
			outData.textureIndices[i] = MadoEngine::TextureManager::GetInstance().GetTextureIndex(outData.textureNames[i]);
		}
	}

	// 全Instanceから共有する不変MeshをUpload Heapへ一度だけ展開
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
