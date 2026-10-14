#pragma once
#include "PlayLogData.h"
#include <filesystem>
#include <nlohmann/json_fwd.hpp>

namespace System {

	/// @brief 確定済みプレイ記録のJSON保存
	class PlayLogJsonWriter final {
	public:
		/// @brief プレイ記録の保存先を設定
		/// @param directory JSONファイルの保存先Directory
		explicit PlayLogJsonWriter(std::filesystem::path directory = "SavedData/PlayLogs");

		/// @brief 一回分の記録を新しいJSONファイルへ保存
		/// @param data 確定済みのプレイ記録
		/// @return 書き込みとファイル確定に成功した場合はtrue
		bool Save(const PlayLogData& data) const;

		/// @brief プレイ記録を表示順を維持するJSONへ変換
		/// @param data 変換対象のプレイ記録
		/// @return 秒数を小数第1位へ四捨五入し、ダメージを整数へ切り捨てた日本語項目名のJSON
		nlohmann::ordered_json ToJson(const PlayLogData& data) const;

	private:
		/// @brief 既存記録と重複しない保存先を生成
		/// @param seed 記録対象のゲームシード
		/// @return 日時とシードを含むJSONファイルのPath
		std::filesystem::path CreateFilePath(std::uint32_t seed) const;

		std::filesystem::path directory_;
	};

}
