#pragma once
#include "BeamEffectTypes.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include <filesystem>
#include <string>

namespace MadoEngine::Beam {

	/// @brief Beamの再生、形状、Noise、Material設定を保持するAsset
	class BeamEffectAsset final : public MadoEngine::Json::IJsonSerializable {
	public:
		static constexpr uint32_t kCurrentVersion = 2;

		/// @brief JsonファイルからAssetを読み込む
		/// @param filePath 読み込むJsonファイル
		/// @return 読み込みに成功した場合はtrue
		bool LoadFromFile(const std::filesystem::path& filePath);

		/// @brief AssetをJsonファイルへ保存する
		/// @param filePath 保存先。空の場合は現在の保存先を使う
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

		/// @brief Beam設定を取得する
		/// @return Beam設定
		const BeamEffectConfig& GetConfig() const {
			return config_;
		}

		/// @brief 編集可能なBeam設定を取得する
		/// @return 編集可能なBeam設定
		BeamEffectConfig& GetConfig() {
			return config_;
		}

		/// @brief 保存先ファイルを取得する
		/// @return 保存先ファイル
		const std::filesystem::path& GetFilePath() const {
			return filePath_;
		}

		/// @brief 保存先ファイルを設定する
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
		BeamEffectConfig config_;
	};

} // namespace MadoEngine::Beam
