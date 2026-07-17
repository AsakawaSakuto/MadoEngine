#pragma once
#include "PrimitiveEffectTypes.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include <filesystem>
#include <string>

namespace MadoEngine::Effect {

	/// @brief Cylinderエフェクトの変更されない設定を保持するAsset
	class CylinderEffectAsset final : public MadoEngine::Json::IJsonSerializable {
	public:
		static constexpr uint32_t kCurrentVersion = 1;

		/// @brief JsonファイルからAssetを読み込む
		/// @param filePath 読み込むJsonファイルパス
		/// @return 読み込みに成功した場合はtrue
		bool LoadFromFile(const std::filesystem::path& filePath);

		/// @brief AssetをJsonファイルへ保存する
		/// @param filePath 保存先。空の場合は読み込み元へ保存する
		/// @param createBackup 上書き前にバックアップを作る場合はtrue
		/// @return 保存に成功した場合はtrue
		bool SaveToFile(const std::filesystem::path& filePath = {}, bool createBackup = true) const;

		/// @brief Jsonから設定を読み込む
		/// @param json 読み込み元Json
		void FromJson(const nlohmann::json& json) override;

		/// @brief 設定をJsonへ変換する
		/// @return 変換されたJson
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

		/// @brief Cylinder設定を取得する
		/// @return Cylinder設定
		const CylinderEffectConfig& GetConfig() const {
			return config_;
		}

		/// @brief 編集可能なCylinder設定を取得する
		/// @return 編集可能なCylinder設定
		CylinderEffectConfig& GetConfig() {
			return config_;
		}

		/// @brief 読み込み元ファイルパスを取得する
		/// @return 読み込み元ファイルパス
		const std::filesystem::path& GetFilePath() const {
			return filePath_;
		}

		/// @brief 設定値を安全な範囲へ補正する
		void Validate();

	private:
		uint32_t version_ = kCurrentVersion;
		std::string name_;
		std::filesystem::path filePath_;
		CylinderEffectConfig config_;
	};

} // namespace MadoEngine::Effect
