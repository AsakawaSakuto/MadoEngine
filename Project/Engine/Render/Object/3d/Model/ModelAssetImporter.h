#pragma once
#include "ModelData.h"
#include "../Animation/AnimationSet.h"
#include <filesystem>
#include <vector>

namespace MadoEngine {

/// @brief GPUリソースから独立した変換済みModelアセット
struct ModelAssetData {
    ModelData modelData;
    AnimationSet animationSet;
};

/// @brief ビルド用変換と実行時復旧で共有するModelインポーター
class ModelAssetImporter {
public:
    /// @brief 元Modelと関連AnimationをEngine用データへ変換
    /// @param sourcePath 元Modelのパス
    /// @param outData 変換結果の出力先
    /// @param dependencies 依存ファイルの収集先、収集不要の場合はnullptr
    /// @return 変換に成功した場合はtrue
    static bool Load(const std::filesystem::path& sourcePath, ModelAssetData& outData,
        std::vector<std::filesystem::path>* dependencies = nullptr);
};

}
