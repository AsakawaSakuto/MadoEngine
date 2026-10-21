#pragma once
#include "ParameterStore.h"
#include <filesystem>
#include <nlohmann/json.hpp>

namespace MadoEngine {

/// @brief 型情報付きパラメータJSONの保存と検証
class ParameterSerializer final {
public:
	/// @brief 全ルートを個別ファイルへ保存
	/// @param store 保存対象の設定値管理
	/// @param directory 保存先ディレクトリ
	/// @return 全ファイルの保存に成功した場合はtrue
	static bool SaveAll(const ParameterStore& store, const std::filesystem::path& directory);

	/// @brief ディレクトリ内の設定を読み込み、不在ファイルを既定値で補完
	/// @param store 読み込み先の設定値管理
	/// @param directory 読み込み元ディレクトリ
	/// @return 全ファイルの検証と読み込みに成功した場合はtrue
	static bool LoadAll(ParameterStore& store, const std::filesystem::path& directory);

	/// @brief 一ルートの設定を型情報付きJSONへ変換
	/// @param store 変換元の設定値管理
	/// @param root 変換するルートグループ
	/// @return スキーマ番号と項目値を含むJSON
	static nlohmann::json ToJson(const ParameterStore& store, const std::string& root);

	/// @brief JSON全体を検証して一ルートへ反映
	/// @param store 読み込み先の設定値管理
	/// @param root 読み込み対象のルートグループ
	/// @param json 型情報付きJSON
	/// @return ルート全体の検証に成功した場合はtrue
	static bool FromJson(ParameterStore& store, const std::string& root, const nlohmann::json& json);

	/// @brief 履歴用に全設定値を文字列へ変換
	/// @param store 変換元の設定値管理
	/// @return UndoとRedoに使用するSnapshot
	static std::string CaptureState(const ParameterStore& store);

	/// @brief 履歴の文字列から設定値を復元
	/// @param store 復元先の設定値管理
	/// @param snapshot 復元するSnapshot
	/// @return Snapshotの検証に成功した場合はtrue
	static bool RestoreState(ParameterStore& store, const std::string& snapshot);
};

}
