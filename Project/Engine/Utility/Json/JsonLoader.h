#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <optional>

/// <summary>
/// JSONファイルの読み書きとエラーハンドリングを隠蔽する静的ユーティリティクラス
/// </summary>
class JsonLoader {
public:
    // データの保存先パスを変更・取得（静的メンバに変更してどこからでもアクセスしやすく）
    static void SetBasePath(const std::string& path);
    static std::string GetBasePath();

    /// <summary>
    /// ファイルからJSONを安全に読み込む（失敗したら std::nullopt を返す）
    /// </summary>
    static std::optional<nlohmann::json> Load(std::string fileName);

    /// <summary>
    /// JSONデータをファイルに綺麗に整形して保存する
    /// </summary>
    static bool Save(std::string fileName, const nlohmann::json& json, int indent = 4);

private:
    inline static std::string s_BasePath = "Assets/Data/Json/"; // デフォルトパス

    // 拡張子のチェックと補完を行うヘルパー
    static std::string FixExtension(std::string fileName);
};