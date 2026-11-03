#include "ModelAssetCatalog.h"
#include "Utility/AssetKey.h"
#include "Utility/Logger/Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {

/// @brief 対応しているModel拡張子か確認
/// @param path 確認するパス
/// @return 対応している場合はtrue
bool IsModelExtension(const std::filesystem::path& path) {
	const std::string extension = MadoEngine::AssetKey::Normalize(path.extension().string());
	return extension == ".obj" || extension == ".gltf" || extension == ".glb";
}

/// @brief Modelの親ディレクトリからModel種別を取得
/// @param modelRoot Modelルート
/// @param path Modelファイルパス
/// @param outType 取得したModel種別
/// @return 種別を取得できた場合はtrue
bool TryGetModelType(const std::filesystem::path& modelRoot, const std::filesystem::path& path, ModelType& outType) {
	std::error_code error;
	const std::filesystem::path relative = std::filesystem::relative(path.parent_path(), modelRoot, error);
	if (error || relative.empty()) {
		return false;
	}

	const std::string firstDirectory = MadoEngine::AssetKey::Normalize((*relative.begin()).string());
	if (firstDirectory == "static") {
		outType = ModelType::Static;
		return true;
	}
	if (firstDirectory == "animation" || firstDirectory == "animated") {
		outType = ModelType::Animated;
		return true;
	}
	if (firstDirectory == "skinning") {
		outType = ModelType::Skinning;
		return true;
	}
	return false;
}

/// @brief Animation専用として参照されるModelファイルを収集
/// @param modelRoot Modelルート
/// @return 通常Model読み込みから除外する正規化済みパス集合
std::unordered_set<std::string, MadoEngine::AssetKey::Hash, MadoEngine::AssetKey::Equal> CollectAnimationOnlyPaths(const std::filesystem::path& modelRoot) {
	std::unordered_set<std::string, MadoEngine::AssetKey::Hash, MadoEngine::AssetKey::Equal> animationOnlyPaths;
	for (auto it = std::filesystem::recursive_directory_iterator(modelRoot);
        it != std::filesystem::recursive_directory_iterator(); ++it) {
        const auto& entry = *it;
        if (entry.is_directory() && MadoEngine::AssetKey::Equals(entry.path().lexically_normal().generic_string(),
            (modelRoot / "Cache").lexically_normal().generic_string())) {

            // 生成物を元アセットとして再走査せず、古い出力による二重登録を防止
            it.disable_recursion_pending();
            continue;
        }
		if (!entry.is_regular_file() ||
			!MadoEngine::AssetKey::Normalize(entry.path().filename().string()).ends_with(".animations.json")) {
			continue;
		}

		std::ifstream manifestFile(entry.path());
		if (!manifestFile.is_open()) {
			continue;
		}

		try {
			nlohmann::json manifestJson;
			manifestFile >> manifestJson;
			const std::string modelFile = manifestJson.value("model", std::string{});
			if (modelFile.empty()) {
				continue;
			}

			const std::filesystem::path manifestDirectory = entry.path().parent_path();
			const std::string baseModelPath = (manifestDirectory / modelFile).lexically_normal().generic_string();
			const auto clipsIterator = manifestJson.find("clips");
			if (clipsIterator == manifestJson.end() || !clipsIterator->is_object()) {
				continue;
			}

			// 基準Modelと異なる参照先だけをAnimation専用Assetとして除外
			for (const auto& [clipName, clipJson] : clipsIterator->items()) {
				(void)clipName;
				if (!clipJson.is_object()) {
					continue;
				}
				const std::string sourceFile = clipJson.value("file", std::string{});
				if (sourceFile.empty()) {
					continue;
				}
				const std::string sourcePath = (manifestDirectory / sourceFile).lexically_normal().generic_string();
				if (!MadoEngine::AssetKey::Equals(sourcePath, baseModelPath)) {
					animationOnlyPaths.insert(sourcePath);
				}
			}
		} catch (const nlohmann::json::exception& exception) {
			Logger::Output(
				"Animation設定の解析に失敗しました: " + entry.path().generic_string() + " / " + exception.what(),
				Logger::Level::Error
			);
		}
	}

	return animationOnlyPaths;
}

} // namespace

namespace MadoEngine {

std::vector<ModelAssetSource> ModelAssetCatalog::Collect(const std::filesystem::path& modelRoot) {
    std::vector<ModelAssetSource> sources;
    if (!std::filesystem::is_directory(modelRoot)) {
        return sources;
    }
    const auto animationOnlyPaths = CollectAnimationOnlyPaths(modelRoot);

    // 変換ツールと実行側で同じ対象を使用し、Animation専用ファイルの重複登録を防止
    for (auto it = std::filesystem::recursive_directory_iterator(modelRoot);
        it != std::filesystem::recursive_directory_iterator(); ++it) {
        const auto& entry = *it;
        if (entry.is_directory() && MadoEngine::AssetKey::Equals(entry.path().lexically_normal().generic_string(),
            (modelRoot / "Cache").lexically_normal().generic_string())) {

            // 生成物を元アセットとして再走査せず、古い出力による二重登録を防止
            it.disable_recursion_pending();
            continue;
        }
        if (!entry.is_regular_file() || !IsModelExtension(entry.path())) {
            continue;
        }
        const auto path = entry.path().lexically_normal();
        if (animationOnlyPaths.contains(path.generic_string())) {
            continue;
        }
        ModelType type = ModelType::Static;
        if (TryGetModelType(modelRoot, path, type)) {
            sources.push_back({ path, type });
        }
    }
    return sources;
}

}