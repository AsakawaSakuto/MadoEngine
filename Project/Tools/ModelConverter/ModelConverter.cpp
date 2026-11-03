#include "Render/Object/3d/Model/ModelAssetCatalog.h"
#include "Render/Object/3d/Model/ModelCache.h"
#include <exception>
#include <filesystem>
#include <iostream>

namespace {

/// @brief 起動時の解析を省略するModelキャッシュのビルド専用変換クラス
class ModelConverter {
public:
    /// @brief 変換対象のディレクトリと変換処理の識別値を保持
    /// @param root Modelルートディレクトリ
    /// @param signature 変換処理の識別値
    ModelConverter(std::filesystem::path root, std::uint64_t signature)
        : root_(std::move(root)), cache_(root_), signature_(signature) {}

    /// @brief 変更されたModelとAnimationを差分変換
    /// @return すべての変換に成功した場合はtrue
    bool Run() const {
        if (!std::filesystem::is_directory(root_)) {
            std::cerr << "Modelディレクトリが見つかりません\n";
            return false;
        }
        std::size_t converted = 0;
        std::size_t skipped = 0;
        bool succeeded = true;
        for (const auto& source : MadoEngine::ModelAssetCatalog::Collect(root_)) {
            MadoEngine::ModelAssetData data;
            std::uint64_t savedSignature = 0;
            if (cache_.Load(source.path, data, &savedSignature) && savedSignature == signature_) {
                ++skipped;
                continue;
            }
            std::vector<std::filesystem::path> dependencies;

            // 実行時と同じImporterを使用し、座標系やAnimation設定の差を防止
            if (!MadoEngine::ModelAssetImporter::Load(source.path, data, &dependencies) ||
                !cache_.Save(source.path, data, dependencies, signature_)) {
                std::cerr << "Modelキャッシュの生成に失敗しました: " << source.path.string() << '\n';
                succeeded = false;
                continue;
            }
            ++converted;
        }
        std::cout << "Modelキャッシュ: 変換 " << converted << " 件、更新なし " << skipped << " 件\n";
        return succeeded;
    }

private:
    std::filesystem::path root_;
    MadoEngine::ModelCache cache_;
    std::uint64_t signature_;
};

}

/// @brief ビルド前にModelと関連Animationのキャッシュを生成
/// @param argc 引数の数
/// @param argv 実行ファイルとModelルートのパス
/// @return 変換に成功した場合は0
int wmain(int argc, wchar_t* argv[]) {
    if (argc != 2) {
        std::cerr << "Modelディレクトリを引数に指定してください\n";
        return 1;
    }
    try {
        std::uint64_t signature = 0;
        if (!MadoEngine::ModelCache::ComputeFingerprint(std::filesystem::absolute(argv[0]), signature)) {
            std::cerr << "Model変換ツールの識別値を取得できません\n";
            return 1;
        }
        const auto root = std::filesystem::absolute(argv[1]).lexically_normal();
        const auto projectDirectory = root.parent_path().parent_path();

        // 日本語を含む配置先でも従来の実行側と同じ相対パスでAssimpへ入力
        std::filesystem::current_path(projectDirectory);
        ModelConverter converter(root.lexically_relative(projectDirectory), signature);
        if (converter.Run()) {
            return 0;
        }
    } catch (const std::exception& error) {
        std::cerr << "Model変換処理でエラーが発生しました: " << error.what() << '\n';
    }
    return 1;
}
