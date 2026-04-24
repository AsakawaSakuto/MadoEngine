#include "AudioManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <algorithm>

namespace MadoEngine {

	AudioManager* AudioManager::GetInstance() {
		static AudioManager instance;
		return &instance;
	}

	void AudioManager::Initialize() {
		Logger::Output("AudioManager : 初期化を開始しました",Logger::Level::Info);

		// マップのクリア
		audioMap_.clear();
		volumeMap_.clear();

		// Assets/Audio ディレクトリ内の全音声ファイルを自動ロード
		std::filesystem::path audioDir(kAudioDirectory);
		if (std::filesystem::exists(audioDir) && std::filesystem::is_directory(audioDir)) {
			Logger::Output("AudioManager : " + std::string(kAudioDirectory) + " から音声ファイルを読み込んでいます", Logger::Level::Info);
			LoadDirectory(audioDir);
			Logger::Output("AudioManager : " + std::to_string(audioMap_.size()) + " 個の音声ファイルを読み込みました", Logger::Level::Info);
		} else {
			Logger::Output("AudioManager : 音声ディレクトリが見つかりません: " + std::string(kAudioDirectory), Logger::Level::Warning);
		}

		Logger::Output("AudioManager : 初期化が完了しました", Logger::Level::Info);
	}

	void AudioManager::LoadDirectory(const std::filesystem::path& directory) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
			if (!entry.is_regular_file()) continue;

			auto ext = entry.path().extension().string();
			// 小文字に変換して比較
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".wav" || ext == ".mp3") {
				Logger::Output("AudioManager : ファイルを読み込んでいます: " + entry.path().string(), Logger::Level::Debug);
				Load(entry.path().string(), 1.0f);
			}
		}
	}

	std::string AudioManager::MakeKey(const std::filesystem::path& filePath) const {
		// ファイル名から拡張子を除いて小文字に変換したものをキーとする
		std::string key = filePath.stem().string();
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		return key;
	}

	void AudioManager::Finalize() {
		Logger::Output("AudioManager : 終了処理を開始しました", Logger::Level::Info);
		StopAll();
		audioMap_.clear();
		volumeMap_.clear();
		Logger::Output("AudioManager : 終了処理が完了しました", Logger::Level::Info);
	}

	void AudioManager::Load(const std::string& filePath, float volume) {
		std::filesystem::path path(filePath);
		std::string key = MakeKey(path);

		// 既に読み込み済みならスキップ
		if (audioMap_.find(key) != audioMap_.end()) {
			Logger::Output("AudioManager : 既に読み込み済みのため、スキップします: " + key, Logger::Level::Warning);
			return;
		}

		auto audio = std::make_unique<AudioX>();
		audio->Initialize(filePath);
		audioMap_[key] = std::move(audio);
		volumeMap_[key] = volume;
		Logger::Output("AudioManager : 音声を読み込みました - キー: " + key + ", パス: " + filePath, Logger::Level::Debug);
	}

	void AudioManager::Play(const std::string& key, bool loop) {
		// キーを小文字に変換して検索
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

		auto it = audioMap_.find(lowerKey);
		if (it == audioMap_.end()) {
			Logger::Output("AudioManager : 音声の再生に失敗しました - キーが見つかりません: " + key, Logger::Level::Error);
			assert(false && "Audio not loaded");
			return;
		}

		float volume = volumeMap_[lowerKey] * masterVolume_;
		Logger::Output("AudioManager : 音声を再生します - キー: " + lowerKey + ", ループ: " + (loop ? "有効" : "無効") + ", 音量: " + std::to_string(volume), Logger::Level::Debug	);
		it->second->PlayAudio(volume, loop);
	}

	void AudioManager::Stop(const std::string& key) {
		// キーを小文字に変換して検索
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

		auto it = audioMap_.find(lowerKey);
		if (it != audioMap_.end() && it->second) {
			Logger::Output("AudioManager : 音声を停止します - キー: " + lowerKey, Logger::Level::Debug);
			it->second->StopAll();
		} else {
			Logger::Output("AudioManager : 音声の停止に失敗しました - キーが見つかりません: " + key, Logger::Level::Warning);
		}
	}

	void AudioManager::StopAll() {
		Logger::Output("AudioManager : 全ての音声を停止します", Logger::Level::Debug);
		for (auto& [key, audio] : audioMap_) {
			if (audio) {
				audio->StopAll();
			}
		}
	}

	void AudioManager::Update() {
		for (auto& [key, audio] : audioMap_) {
			if (audio) {
				audio->SetVolume(volumeMap_[key] * masterVolume_);
				audio->Update();
			}
		}
	}

	void AudioManager::SetMasterVolume(float volume) {
		Logger::Output("AudioManager : マスターボリュームを " + std::to_string(volume) + " に設定しました", Logger::Level::Debug);
		masterVolume_ = volume;
	}

	void AudioManager::SetVolume(const std::string& key, float volume) {
		// キーを小文字に変換して検索
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

		auto it = volumeMap_.find(lowerKey);
		if (it != volumeMap_.end()) {
			Logger::Output("AudioManager : " + lowerKey + " の音量を " + std::to_string(volume) + " に設定しました", Logger::Level::Debug);
			it->second = volume;
			// 即座に反映
			auto audioIt = audioMap_.find(lowerKey);
			if (audioIt != audioMap_.end() && audioIt->second) {
				audioIt->second->SetVolume(volume * masterVolume_);
			}
		} else {
			Logger::Output("AudioManager	: 音量の設定に失敗しました - キーが見つかりません: " + key, Logger::Level::Warning);
		}
	}

	float AudioManager::GetVolume(const std::string& key) const {
		// キーを小文字に変換して検索
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

		auto it = volumeMap_.find(lowerKey);
		if (it != volumeMap_.end()) {
			return it->second;
		}
		return 1.0f;
	}

	bool AudioManager::IsLoaded(const std::string& key) const {
		// キーを小文字に変換して検索
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
		return audioMap_.find(lowerKey) != audioMap_.end();
	}

	std::vector<std::string> AudioManager::GetAllKeys() const {
		std::vector<std::string> keys;
		keys.reserve(audioMap_.size());
		for (const auto& [key, _] : audioMap_) {
			keys.push_back(key);
		}
		return keys;
	}

} // namespace MadoEngine