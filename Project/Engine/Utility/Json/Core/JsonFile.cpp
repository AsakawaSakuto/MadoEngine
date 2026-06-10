#include "JsonFile.h"

#include "Utility/Logger/Logger.h"
#include <fstream>

namespace {

	/// @brief ログ出力用のパス文字列を取得する
	/// @param filePath 変換するファイルパス
	/// @return ログに使用する文字列
	std::string ToLogPath(const std::filesystem::path& filePath) {
		return filePath.generic_string();
	}

}

namespace MadoEngine::Json {

	bool JsonFile::Exists(const std::filesystem::path& filePath) {
		return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
	}

	bool JsonFile::Load(const std::filesystem::path& filePath, nlohmann::json& outJson) {
		if (!Exists(filePath)) {
			Logger::Output("Jsonファイルが見つかりません : " + ToLogPath(filePath), Logger::Level::Warning);
			return false;
		}

		try {
			std::ifstream file(filePath);
			if (!file.is_open()) {
				Logger::Output("Jsonファイルを開けませんでした : " + ToLogPath(filePath), Logger::Level::Error);
				return false;
			}

			file >> outJson;
			Logger::Output("Jsonファイルを読み込みました : " + ToLogPath(filePath), Logger::Level::Assets);
			return true;
		}
		catch (const nlohmann::json::exception& e) {
			Logger::Output("Jsonの解析に失敗しました : " + ToLogPath(filePath) + " / " + e.what(), Logger::Level::Error);
		}
		catch (const std::exception& e) {
			Logger::Output("Jsonファイルの読み込み中に例外が発生しました : " + ToLogPath(filePath) + " / " + e.what(), Logger::Level::Error);
		}

		return false;
	}

	bool JsonFile::Save(
		const std::filesystem::path& filePath,
		const nlohmann::json& json,
		int indent,
		bool createBackup
	) {
		try {
			const std::filesystem::path parentPath = filePath.parent_path();
			if (!parentPath.empty()) {
				std::filesystem::create_directories(parentPath);
			}

			if (createBackup && Exists(filePath)) {
				CreateBackup(filePath);
			}

			std::ofstream file(filePath);
			if (!file.is_open()) {
				Logger::Output("Jsonファイルを保存用に開けませんでした : " + ToLogPath(filePath), Logger::Level::Error);
				return false;
			}

			file << json.dump(indent);
			Logger::Output("Jsonファイルを保存しました : " + ToLogPath(filePath), Logger::Level::Assets);
			return true;
		}
		catch (const std::exception& e) {
			Logger::Output("Jsonファイルの保存中に例外が発生しました : " + ToLogPath(filePath) + " / " + e.what(), Logger::Level::Error);
		}

		return false;
	}

	bool JsonFile::CreateBackup(const std::filesystem::path& filePath) {
		if (!Exists(filePath)) {
			return false;
		}

		try {
			std::filesystem::path backupPath = filePath;
			backupPath += ".bak";
			std::filesystem::copy_file(
				filePath,
				backupPath,
				std::filesystem::copy_options::overwrite_existing
			);

			Logger::Output("Jsonファイルのバックアップを作成しました : " + ToLogPath(backupPath), Logger::Level::Assets);
			return true;
		}
		catch (const std::exception& e) {
			Logger::Output("Jsonファイルのバックアップ作成に失敗しました : " + ToLogPath(filePath) + " / " + e.what(), Logger::Level::Warning);
		}

		return false;
	}

}
