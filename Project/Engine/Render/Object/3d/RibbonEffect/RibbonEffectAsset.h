#pragma once
#include "RibbonEffectTypes.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include <filesystem>
#include <string>

namespace MadoEngine::Ribbon {

	/// @brief Ribbon描画とPoint生成の変更されない設定を保持するAsset
	class RibbonEffectAsset final : public MadoEngine::Json::IJsonSerializable {
	public:
		static constexpr uint32_t kCurrentVersion = 5;

		/// @brief JsonファイルからAssetを読み込む
		/// @param filePath 読み込むJsonファイル
		/// @return 読み込みに成功した場合はtrue
		bool LoadFromFile(const std::filesystem::path& filePath);

		/// @brief AssetをJsonファイルへ保存する
		/// @param filePath 保存先。空の場合は読み込み元へ保存する
		/// @param createBackup 上書き前にBackupを作成する場合はtrue
		/// @return 保存に成功した場合はtrue
		bool SaveToFile(const std::filesystem::path& filePath = {}, bool createBackup = true) const;

		/// @brief Jsonから設定を読み込む
		/// @param json 読み込み元Json
		void FromJson(const nlohmann::json& json) override;

		/// @brief 設定をJsonへ変換する
		/// @return 変換後Json
		nlohmann::json ToJson() const override;

		/// @brief Asset名を取得する
		/// @return Asset名
		const std::string& GetName() const {
			return name_;
		}

		/// @brief Asset名を設定する
		/// @param name 新しいAsset名
		void SetName(const std::string& name) {
			name_ = name;
		}

		/// @brief Ribbon設定を取得する
		/// @return Ribbon設定
		const RibbonEffectConfig& GetConfig() const {
			return emitters_.front();
		}

		/// @brief 編集可能なRibbon設定を取得する
		/// @return 編集可能なRibbon設定
		RibbonEffectConfig& GetConfig() {
			return emitters_.front();
		}

		/// @brief Emitter設定一覧を取得する
		/// @return Emitter設定一覧
		const std::vector<RibbonEmitterConfig>& GetEmitters() const {
			return emitters_;
		}

		/// @brief 編集可能なEmitter設定一覧を取得する
		/// @return 編集可能なEmitter設定一覧
		std::vector<RibbonEmitterConfig>& GetEmitters() {
			return emitters_;
		}

		/// @brief Assetの保存先を取得する
		/// @return 保存先ファイル
		const std::filesystem::path& GetFilePath() const {
			return filePath_;
		}

		/// @brief Assetの保存先を設定する
		/// @param filePath 保存先ファイル
		void SetFilePath(const std::filesystem::path& filePath) {
			filePath_ = filePath;
		}

		/// @brief 設定値を安全な範囲へ補正する
		void Validate();

	private:
		uint32_t version_ = kCurrentVersion;
		std::string name_;
		std::filesystem::path filePath_;
		std::vector<RibbonEmitterConfig> emitters_{ RibbonEmitterConfig{} };
	};

} // namespace MadoEngine::Ribbon
