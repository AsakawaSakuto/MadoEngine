#pragma once
#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include <vector>
#include "Data/Audio.h"

namespace MadoEngine {

	/// @brief オーディオ管理クラス（シングルトン）
	/// Assets/Audio内の音声ファイルを自動管理し、文字列キーで再生可能
	class AudioManager {
	public:

		/// @brief シングルトンインスタンスを取得
		/// @return AudioManagerのインスタンスポインタ
		static AudioManager* GetInstance();

		/// @brief 初期化（Assets/Audio内の全wav/mp3を自動ロード）
		void Initialize();

		/// @brief 終了処理（全音声を停止して解放）
		void Finalize();

		/// @brief 音声ファイルを手動で追加読み込み
		/// @param filePath 音声ファイルのパス
		/// @param volume 個別音量（0.0f～1.0f、デフォルト: 1.0f）
		void Load(const std::string& filePath, float volume = 1.0f);

		/// @brief 音声を再生
		/// @param key ファイル名（拡張子なし）。例: "fire.mp3" → "fire"
		/// @param loop ループ再生するか（デフォルト: false）
		void Play(const std::string& key, bool loop = false);

		/// @brief 特定の音声を停止
		/// @param key ファイル名（拡張子なし）
		void Stop(const std::string& key);

		/// @brief 全ての音声を停止
		void StopAll();

		/// @brief 毎フレーム更新（終了したインスタンスのクリーンアップ）
		void Update();

		/// @brief 音声が読み込み済みか確認
		/// @param key ファイル名（拡張子なし）
		/// @return 読み込み済みならtrue
		bool IsLoaded(const std::string& key) const;

		/// @brief デストラクタ
		~AudioManager() = default;

		/// @brief マスターボリュームを設定
		/// @param volume 音量（0.0f～1.0f）
		void SetMasterVolume(float volume);

		/// @brief マスターボリュームを取得
		/// @return 現在のマスターボリューム
		float GetMasterVolume() const { return masterVolume_; }

		/// @brief 個別の音声の音量を設定
		/// @param key ファイル名（拡張子なし）
		/// @param volume 音量（0.0f～1.0f）
		void SetVolume(const std::string& key, float volume);

		/// @brief 個別の音声の音量を取得
		/// @param key ファイル名（拡張子なし）
		/// @return 音量（見つからない場合は1.0f）
		float GetVolume(const std::string& key) const;

		/// @brief 読み込み済みの全キーを取得
		/// @return キーのリスト
		std::vector<std::string> GetAllKeys() const;

	private:
		AudioManager() = default;
		AudioManager(const AudioManager&) = delete;
		AudioManager& operator=(const AudioManager&) = delete;

		/// @brief ディレクトリ内の音声ファイルを再帰的にロード
		/// @param directory 読み込むディレクトリパス
		void LoadDirectory(const std::filesystem::path& directory);

		/// @brief ファイルパスからキー名を生成（拡張子なしのファイル名）
		/// @param filePath ファイルパス
		/// @return キー名（拡張子なし）
		std::string MakeKey(const std::filesystem::path& filePath) const;

		// 音声データ管理（キー → AudioX）
		std::unordered_map<std::string, std::unique_ptr<AudioX>> audioMap_;

		// 個別音量管理（キー → 音量）
		std::unordered_map<std::string, float> volumeMap_;

		// マスターボリューム
		float masterVolume_ = 0.5f;

		// 音声ファイルの基準ディレクトリ
		static constexpr const char* kAudioDirectory = "Assets/Audio";
	};

} // namespace MadoEngine