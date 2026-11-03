#pragma once
#include <assimp/DefaultIOSystem.h>
#include <assimp/Importer.hpp>
#include <algorithm>
#include <filesystem>
#include <vector>

namespace MadoEngine {

/// @brief Assimpが参照したファイルと欠損ファイルの依存関係を収集する入出力クラス
class ModelImportIO final : public Assimp::DefaultIOSystem {
public:
    /// @brief 依存ファイルの収集先を保持
    /// @param dependencies 収集先のパス配列
    explicit ModelImportIO(std::vector<std::filesystem::path>& dependencies) : dependencies_(dependencies) {}

    /// @brief 存在確認したパスを依存関係へ記録
    /// @param pFile 確認対象のファイルパス
    /// @return ファイルが存在する場合はtrue
    bool Exists(const char* pFile) const override {
        Record(pFile);
        return Assimp::DefaultIOSystem::Exists(pFile);
    }

    /// @brief 読み込みを試みたパスを依存関係へ記録
    /// @param pFile 対象のファイルパス
    /// @param pMode ファイルのオープンモード
    /// @return 開いたストリーム、失敗時はnullptr
    Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
        Record(pFile);
        return Assimp::DefaultIOSystem::Open(pFile, pMode);
    }

    /// @brief 必要な場合だけImporterへ収集用入出力を設定
    /// @param importer 設定対象のImporter
    /// @param dependencies 収集先、収集不要の場合はnullptr
    static void Attach(Assimp::Importer& importer, std::vector<std::filesystem::path>* dependencies) {
        if (dependencies) {
            importer.SetIOHandler(new ModelImportIO(*dependencies));
        }
    }

private:
    /// @brief 重複を除外した絶対パスを記録
    /// @param file 記録対象のファイルパス
    void Record(const char* file) const {
        const auto path = std::filesystem::absolute(file).lexically_normal();
        if (std::find(dependencies_.begin(), dependencies_.end(), path) == dependencies_.end()) {

            // 欠損MTLなどが後から追加された場合も再生成できるよう、未存在の参照先も保持
            dependencies_.push_back(path);
        }
    }

    std::vector<std::filesystem::path>& dependencies_;
};

}
