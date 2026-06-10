#pragma once
#include "../Core/IJsonSerializable.h"
#include <filesystem>
#include <string>

namespace MadoEngine::Json {

	/// @brief Window関連設定
	struct WindowConfig : public IJsonSerializable {
		std::string title = "MadoEngine";
		std::string iconPath = "";
		int width = 1280;
		int height = 720;
		bool isResizable = true;
		bool isShowMouseCursor = true;
		bool isFullscreen = false;
		bool isVsync = true;

		/// @brief JsonからWindow設定を読み込む
		/// @param json 読み込み元のJson
		void FromJson(const nlohmann::json& json) override;

		/// @brief Window設定をJsonへ変換する
		/// @return 変換されたJson
		nlohmann::json ToJson() const override;
	};

	/// @brief エンジン設定やアプリ設定のJson管理クラス
	class ConfigManager {
	public:
		/// @brief 設定ファイルを読み込む
		/// @param filePath 読み込む設定ファイル
		/// @return 読み込みに成功した場合はtrue
		bool Load(const std::filesystem::path& filePath);

		/// @brief 設定ファイルを保存する
		/// @param createBackup 上書き前にバックアップを作る場合はtrue
		/// @return 保存に成功した場合はtrue
		bool Save(bool createBackup = true) const;

		/// @brief 設定ファイルを指定パスへ保存する
		/// @param filePath 保存先の設定ファイル
		/// @param createBackup 上書き前にバックアップを作る場合はtrue
		/// @return 保存に成功した場合はtrue
		bool SaveAs(const std::filesystem::path& filePath, bool createBackup = true);

		/// @brief 設定を初期化する
		void Clear();

		/// @brief Window設定を取得する
		/// @param defaultValue 設定が存在しない場合の値
		/// @return Window設定
		WindowConfig GetWindowConfig(const WindowConfig& defaultValue = {}) const;

		/// @brief Window設定を登録する
		/// @param config 登録するWindow設定
		void SetWindowConfig(const WindowConfig& config);

		/// @brief 指定セクションが存在するか確認する
		/// @param sectionName セクション名
		/// @return 存在する場合はtrue
		bool ContainsSection(const std::string& sectionName) const;

		/// @brief 指定セクションのJsonを取得する
		/// @param sectionName セクション名
		/// @return セクションのJson
		const nlohmann::json& GetSection(const std::string& sectionName) const;

		/// @brief 指定セクションのJsonを設定する
		/// @param sectionName セクション名
		/// @param json 設定するJson
		void SetSection(const std::string& sectionName, const nlohmann::json& json);

		/// @brief 指定キーの値を取得する
		/// @param sectionName セクション名
		/// @param key キー名
		/// @param defaultValue 値が存在しない場合の値
		/// @return 取得した値
		template <class T>
		T GetValue(const std::string& sectionName, const std::string& key, const T& defaultValue) const {
			if (!ContainsSection(sectionName)) {
				return defaultValue;
			}

			const nlohmann::json& section = root_.at(sectionName);
			if (!section.is_object() || !section.contains(key)) {
				return defaultValue;
			}

			try {
				return section.at(key).get<T>();
			}
			catch (const nlohmann::json::exception&) {
				return defaultValue;
			}
		}

		/// @brief 指定キーの値を設定する
		/// @param sectionName セクション名
		/// @param key キー名
		/// @param value 設定する値
		template <class T>
		void SetValue(const std::string& sectionName, const std::string& key, const T& value) {
			root_[sectionName][key] = value;
		}

		/// @brief 管理中のJsonを取得する
		/// @return 管理中のJson
		const nlohmann::json& GetRoot() const { return root_; }

	private:
		std::filesystem::path filePath_;
		nlohmann::json root_ = nlohmann::json::object();
	};

}
