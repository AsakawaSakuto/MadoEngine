#include "JsonLoader.h"
#include <fstream>
#include <filesystem>
#include "Utility/Logger/Logger.h"

void JsonLoader::SetBasePath(const std::string& path) {
    s_BasePath = path;
}

std::string JsonLoader::GetBasePath() {
    return s_BasePath;
}

std::string JsonLoader::FixExtension(std::string fileName) {
    if (fileName.size() < 5 || fileName.substr(fileName.size() - 5) != ".json") {
        fileName += ".json";
    }
    return fileName;
}

std::optional<nlohmann::json> JsonLoader::Load(std::string fileName) {
    fileName = FixExtension(fileName);
    std::filesystem::path fullPath = s_BasePath + fileName;

    // ファイル存在チェック
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        // ここで「ファイルが見つかりません」などのエラーログをImGuiコンソール等に出すとデバッグが快適になります
		Logger::Output("JsonLoader : ファイルを開けませんでした: " + fullPath.string(), Logger::Level::Error);
        return std::nullopt;
    }

    try {
        nlohmann::json j;
        file >> j; // nlohmann/json の超強力な自動パース
        return j;
    }
    catch (const nlohmann::json::parse_error& e) {
        // カンマ忘れなどの構文エラーがあっても、ゲームをクラッシュさせずにエラーログで受け止める
        Logger::Output("JsonLoader : JSONパースエラー: " + std::string(e.what()), Logger::Level::Error);
        return std::nullopt;
    }
}

bool JsonLoader::Save(std::string fileName, const nlohmann::json& json, int indent) {
    fileName = FixExtension(fileName);
    std::filesystem::path fullPath = s_BasePath + fileName;
    std::filesystem::path directory = fullPath.parent_path();

    // 【過去の素晴らしい設計を踏襲】ディレクトリが存在しない場合は自動作成
    if (!directory.empty() && !std::filesystem::exists(directory)) {
        try {
            std::filesystem::create_directories(directory);
        }
        catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    std::ofstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }

    try {
        // dump(indent) で綺麗にインデントされた人間が読めるJSONとして書き出す
        file << json.dump(indent);
        return true;
    }
    catch (...) {
        return false;
    }
}