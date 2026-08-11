#include "JsonFile.h"

#include "Utility/Logger/Logger.h"
#include <fstream>

namespace {

	/// @brief ログ出力用のパス文字列を取得
	/// @param filePath 変換するファイルパス
	/// @return ログに使用する文字列
	std::string ToLogPath(const std::filesystem::path& filePath) {
		const std::u8string value = filePath.generic_u8string();
		return std::string(
			reinterpret_cast<const char*>(value.data()),
			value.size()
		);
	}

}

namespace MadoEngine::Json {

	bool JsonFile::Exists(const std::filesystem::path& filePath) {
		return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
	}

	bool JsonFile::Load(const std::filesystem::path& filePath, nlohmann::json& outJson) {

		// File I/OとJson Parseの例外を境界内で処理して呼び出し側へboolで通知
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

		// 上書き前のBackupと親Directory生成を同じ保存処理内で実施
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

		// Backup名を固定して世代増加を避けつつ直前内容だけを保持
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
