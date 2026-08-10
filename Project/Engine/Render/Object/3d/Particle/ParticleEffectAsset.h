#pragma once
#include "ParticleTypes.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include <filesystem>
#include <string>
#include <vector>

namespace MadoEngine::Particle {

	/// @brief パーティクルエフェクトの変更されない設定を保持するAsset
	class ParticleEffectAsset final : public MadoEngine::Json::IJsonSerializable {
	public:
		static constexpr uint32_t kCurrentVersion = 2;

		/// @brief JsonファイルからAssetを読み込み
		/// @param filePath 読み込むJsonファイルパス
		/// @return 読み込みに成功した場合はtrue
		bool LoadFromFile(const std::filesystem::path& filePath);

		/// @brief AssetをJsonファイルへ保存
		/// @param filePath 保存先、空の場合は読み込み元へ保存
		/// @param createBackup 上書き前にバックアップを作る場合はtrue
		/// @return 保存に成功した場合はtrue
		bool SaveToFile(const std::filesystem::path& filePath = {}, bool createBackup = true) const;

		/// @brief JsonからAsset設定を読み込み
		/// @param json 読み込み元Json
		void FromJson(const nlohmann::json& json) override;

		/// @brief Asset設定をJsonへ変換
		/// @return 変換したJson
		nlohmann::json ToJson() const override;

		/// @brief Asset名を取得
		/// @return Asset名
		const std::string& GetName() const { return name_; }

		/// @brief Asset名を設定
		/// @param name 設定するAsset名
		void SetName(const std::string& name) { name_ = name; }

		/// @brief Emitter設定一覧を取得
		/// @return Emitter設定一覧
		const std::vector<EmitterConfig>& GetEmitters() const { return emitters_; }

		/// @brief Emitter設定一覧を取得
		/// @return 編集可能なEmitter設定一覧
		std::vector<EmitterConfig>& GetEmitters() { return emitters_; }

		/// @brief 読み込み元ファイルパスを取得
		/// @return 読み込み元ファイルパス
		const std::filesystem::path& GetFilePath() const { return filePath_; }

		/// @brief 設定値を安全な範囲へ補正
		void Validate();

	private:
		uint32_t version_ = kCurrentVersion;
		std::string name_;
		std::filesystem::path filePath_;
		std::vector<EmitterConfig> emitters_;
	};

} // namespace MadoEngine::Particle
