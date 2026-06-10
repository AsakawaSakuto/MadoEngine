#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>

namespace MadoEngine::Json {

	/// @brief Jsonファイルの読み込みと保存を担当するユーティリティ
	class JsonFile {
	public:
		/// @brief Jsonファイルが存在するか確認する
		/// @param filePath 確認するファイルパス
		/// @return ファイルが存在する場合はtrue
		static bool Exists(const std::filesystem::path& filePath);

		/// @brief Jsonファイルを読み込む
		/// @param filePath 読み込むファイルパス
		/// @param outJson 読み込んだJsonの出力先
		/// @return 読み込みに成功した場合はtrue
		static bool Load(const std::filesystem::path& filePath, nlohmann::json& outJson);

		/// @brief Jsonファイルを保存する
		/// @param filePath 保存先のファイルパス
		/// @param json 保存するJson
		/// @param indent インデント幅
		/// @param createBackup 上書き前にバックアップを作る場合はtrue
		/// @return 保存に成功した場合はtrue
		static bool Save(
			const std::filesystem::path& filePath,
			const nlohmann::json& json,
			int indent = 4,
			bool createBackup = false
		);

		/// @brief Jsonファイルのバックアップを作成する
		/// @param filePath バックアップ元のファイルパス
		/// @return バックアップに成功した場合はtrue
		static bool CreateBackup(const std::filesystem::path& filePath);
	};

}
