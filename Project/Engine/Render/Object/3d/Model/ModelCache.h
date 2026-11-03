#pragma once
#include "ModelAssetImporter.h"
#include <cstdint>
#include <filesystem>
#include <vector>

namespace MadoEngine {

/// @brief 変換済みModelの保存と依存関係の検証を担当するディスクキャッシュ
class ModelCache {
public:
    /// @brief 元Modelの基準ディレクトリを保持
    /// @param modelRoot Modelのルートディレクトリ
    explicit ModelCache(std::filesystem::path modelRoot = "Assets/Model");

    /// @brief 元Modelに対応するキャッシュパスを取得
    /// @param sourcePath 元Modelのパス
    /// @return キャッシュパス、基準ディレクトリ外の場合は空のパス
    std::filesystem::path GetPath(const std::filesystem::path& sourcePath) const;

    /// @brief 形式と依存関係を検証して変換済みModelを復元
    /// @param sourcePath 元Modelのパス
    /// @param outData 復元したデータの出力先
    /// @param converterSignature 保存時の変換ツール識別値の出力先、不要の場合はnullptr
    /// @return キャッシュが有効で復元できた場合はtrue
    bool Load(const std::filesystem::path& sourcePath, ModelAssetData& outData,
        std::uint64_t* converterSignature = nullptr) const;

    /// @brief 依存関係と変換済みModelを完成後に一括置換して保存
    /// @param sourcePath 元Modelのパス
    /// @param data 保存する変換済みデータ
    /// @param dependencies 元Modelと関連ファイルのパス集合
    /// @param converterSignature 変換ツールの識別値
    /// @return 保存に成功した場合はtrue
    bool Save(const std::filesystem::path& sourcePath, const ModelAssetData& data,
        const std::vector<std::filesystem::path>& dependencies, std::uint64_t converterSignature) const;

    /// @brief ファイル内容から更新判定用の識別値を計算
    /// @param path 対象ファイルのパス
    /// @param outFingerprint 計算結果の出力先
    /// @return 読み込みに成功した場合はtrue
    static bool ComputeFingerprint(const std::filesystem::path& path, std::uint64_t& outFingerprint);

private:
    std::filesystem::path modelRoot_;
};

}
