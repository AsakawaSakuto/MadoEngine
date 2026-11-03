#pragma once
#include "ModelType.h"
#include <filesystem>
#include <vector>

namespace MadoEngine {

/// @brief 読み込み対象のModelファイルと描画種別
struct ModelAssetSource {
    std::filesystem::path path;
    ModelType type = ModelType::Static;
};

/// @brief 通常ModelとAnimation専用ファイルを区別するアセット一覧
class ModelAssetCatalog {
public:
    /// @brief Modelディレクトリから描画対象のアセットを収集
    /// @param modelRoot Modelのルートディレクトリ
    /// @return パスと描画種別の一覧
    static std::vector<ModelAssetSource> Collect(const std::filesystem::path& modelRoot);
};

}
