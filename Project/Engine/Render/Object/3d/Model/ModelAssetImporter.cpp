#include "ModelAssetImporter.h"
#include "ModelImportIO.h"
#include "../Animation/AnimationFunction.h"
#include "Utility/AssetKey.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <utility>

AnimationClip LoadAnimationFile(const std::string& filename, int index, std::vector<std::filesystem::path>* dependencies) {
    AnimationClip animation;
    Assimp::Importer importer;
    MadoEngine::ModelImportIO::Attach(importer, dependencies);
    const aiScene* scene = importer.ReadFile(filename.c_str(),
        MadoEngine::ModelImportSettings::kCoordinateSystemPostProcess);
    if (!scene || scene->mNumAnimations == 0) {
        return animation;
    }

    // 欠損Indexの扱いを従来経路と統一し、存在するClipの範囲へ制限
    index = std::clamp(index, 0, static_cast<int>(scene->mNumAnimations - 1));
    const aiAnimation* sourceAnimation = scene->mAnimations[index];
    double ticksPerSecond = 1.0;
    if (sourceAnimation->mTicksPerSecond != 0.0) {
        ticksPerSecond = sourceAnimation->mTicksPerSecond;
    }
    animation.duration = static_cast<float>(sourceAnimation->mDuration / ticksPerSecond);

    // 左手系へ変換済みのTrackを秒単位へ統一し、再生側の評価形式に固定
    for (std::uint32_t channelIndex = 0; channelIndex < sourceAnimation->mNumChannels; ++channelIndex) {
        const aiNodeAnim* sourceNode = sourceAnimation->mChannels[channelIndex];
        NodeAnimation& node = animation.nodeAnimations[sourceNode->mNodeName.C_Str()];
        for (std::uint32_t keyIndex = 0; keyIndex < sourceNode->mNumPositionKeys; ++keyIndex) {
            const auto& key = sourceNode->mPositionKeys[keyIndex];
            node.translate.keyframes.push_back({
                static_cast<float>(key.mTime / ticksPerSecond),
                { key.mValue.x, key.mValue.y, key.mValue.z } });
        }
        for (std::uint32_t keyIndex = 0; keyIndex < sourceNode->mNumRotationKeys; ++keyIndex) {
            const auto& key = sourceNode->mRotationKeys[keyIndex];
            node.rotate.keyframes.push_back({
                static_cast<float>(key.mTime / ticksPerSecond),
                { key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w } });
        }
        for (std::uint32_t keyIndex = 0; keyIndex < sourceNode->mNumScalingKeys; ++keyIndex) {
            const auto& key = sourceNode->mScalingKeys[keyIndex];
            node.scale.keyframes.push_back({
                static_cast<float>(key.mTime / ticksPerSecond),
                { key.mValue.x, key.mValue.y, key.mValue.z } });
        }
    }
    return animation;
}

namespace {

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
	if (std::isfinite(playbackSpeed)) {
		clip.playbackSpeed = (std::max)(0.0f, playbackSpeed);
	} else {
		clip.playbackSpeed = 1.0f;
	}
	const float blendDuration = json.value("blendDuration", 0.15f);
	if (std::isfinite(blendDuration)) {
		clip.blendDuration = (std::max)(0.0f, blendDuration);
	} else {
		clip.blendDuration = 0.15f;
	}
}

/// @brief Modelに対応するAnimationSetを読み込み
/// @param modelPath Modelファイルパス
/// @param rootNode 互換性検証の基準になるRootNode
/// @param outAnimationSet 読み込んだAnimationSetの出力先
/// @param dependencies 依存ファイルの収集先、収集不要の場合はnullptr
void LoadAnimationSet(
	const std::filesystem::path& modelPath,
	const ModelNode& rootNode,
	AnimationSet& outAnimationSet,
    std::vector<std::filesystem::path>* dependencies) {
	const std::filesystem::path manifestPath =
		modelPath.parent_path() / (modelPath.stem().string() + ".animations.json");
	if (dependencies) {

        // 設定ファイルの追加と削除でも既存キャッシュを無効化できるよう存在前から追跡
        dependencies->push_back(std::filesystem::absolute(manifestPath).lexically_normal());
    }
    if (!std::filesystem::exists(manifestPath)) {
		AnimationClip clip = LoadAnimationFile(modelPath.generic_string(), 0, dependencies);
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
			AnimationClip clip = LoadAnimationFile(sourcePath.generic_string(), animationIndex, dependencies);
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

namespace MadoEngine {

bool ModelAssetImporter::Load(const std::filesystem::path& sourcePath, ModelAssetData& outData,
    std::vector<std::filesystem::path>* dependencies) {
    try {
        ModelAssetData data;
        data.modelData = LoadObject3dFile(sourcePath.generic_string(), dependencies);
        const std::string extension = AssetKey::Normalize(sourcePath.extension().string());
        if (extension == ".gltf" || extension == ".glb") {

            // メッシュと同じ変換規約でAnimationも保持し、実行時の再解析を省略
            LoadAnimationSet(sourcePath, data.modelData.rootNode, data.animationSet, dependencies);
        }
        outData = std::move(data);
        return true;
    } catch (const std::exception& error) {
        Logger::Output("Modelの変換に失敗しました: " + sourcePath.generic_string() + " / " + error.what(), Logger::Level::Error);
        return false;
    }
}

}