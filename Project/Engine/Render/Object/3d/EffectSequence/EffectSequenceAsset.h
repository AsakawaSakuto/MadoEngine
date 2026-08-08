#pragma once
#include "EffectSequenceTypes.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include <filesystem>
#include <string>

namespace MadoEngine::EffectSequence {

	/// @brief 複数Effectの時間軸設定を保持するAsset
	class EffectSequenceAsset final : public MadoEngine::Json::IJsonSerializable {
	public:
		static constexpr uint32_t kCurrentVersion = 1;

		/// @brief JSONファイルからAssetを読み込む
		/// @param filePath 読み込むJSONファイル
		/// @return 読み込みに成功した場合はtrue
		bool LoadFromFile(const std::filesystem::path& filePath);

		/// @brief AssetをJSONファイルへ保存する
		/// @param filePath 保存先。空の場合は現在の保存先を使用する
		/// @param createBackup 上書き前にBackupを作る場合はtrue
		/// @return 保存に成功した場合はtrue
		bool SaveToFile(const std::filesystem::path& filePath = {}, bool createBackup = true) const;

		/// @brief JSONからSequence設定を読み込む
		/// @param json 読み込み元JSON
		void FromJson(const nlohmann::json& json) override;

		/// @brief Sequence設定をJSONへ変換する
		/// @return 変換後のJSON
		nlohmann::json ToJson() const override;

		/// @brief Asset名を取得する
		/// @return Asset名
		const std::string& GetName() const {
			return name_;
		}

		/// @brief Asset名を設定する
		/// @param name 設定するAsset名
		void SetName(const std::string& name) {
			name_ = name;
		}

		/// @brief Sequence設定を取得する
		/// @return Sequence設定
		const EffectSequenceConfig& GetConfig() const {
			return config_;
		}

		/// @brief 編集可能なSequence設定を取得する
		/// @return 編集可能なSequence設定
		EffectSequenceConfig& GetConfig() {
			return config_;
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

		/// @brief 使用されていない安定Node IDを生成する
		/// @return 使用可能なNode ID
		uint32_t GenerateNodeId() const;

		/// @brief Sequence設定を安全な範囲へ補正する
		void Validate();

	private:
		uint32_t version_ = kCurrentVersion;
		std::string name_;
		std::filesystem::path filePath_;
		EffectSequenceConfig config_;
	};

} // namespace MadoEngine::EffectSequence
