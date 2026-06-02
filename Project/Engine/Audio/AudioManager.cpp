#include "AudioManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <algorithm>

namespace MadoEngine {

	AudioManager& AudioManager::GetInstance() {
		static AudioManager instance;
		return instance;
	}

	void AudioManager::Initialize() {
		Logger::Output("AudioManager : 初期化を開始しました",Logger::Level::Engine);

		// マップのクリア
		audioMap_.clear();
		volumeMap_.clear();

		// Assets/Audio ディレクトリ内の全音声ファイルを自動ロード
		std::filesystem::path audioDir(kAudioDirectory);
		if (std::filesystem::exists(audioDir) && std::filesystem::is_directory(audioDir)) {
			Logger::Output("AudioManager : " + std::string(kAudioDirectory) + " から音声ファイルを読み込んでいます", Logger::Level::Engine);
			LoadDirectory(audioDir);
			Logger::Output("AudioManager : " + std::to_string(audioMap_.size()) + " 個の音声ファイルを読み込みました", Logger::Level::Engine);
		} else {
			Logger::Output("AudioManager : 音声ディレクトリが見つかりません: " + std::string(kAudioDirectory), Logger::Level::Warning);
		}

		Logger::Output("AudioManager : 初期化が完了しました", Logger::Level::Engine);
	}

	void AudioManager::LoadDirectory(const std::filesystem::path& directory) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
			if (!entry.is_regular_file()) continue;

			auto ext = entry.path().extension().string();
			// 小文字に変換して比較
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".wav" || ext == ".mp3") {
				AudioType type = DetermineTypeFromPath(entry.path());
				Logger::Output("AudioManager : ファイルを読み込んでいます: " + entry.path().string() + " [" + AudioTypeToString(type) + "]", Logger::Level::Assets);
				Load(entry.path().string(), type, 1.0f);
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
		Logger::Output("AudioManager : 終了処理を開始しました", Logger::Level::Engine);
		StopAll();
		audioMap_.clear();
		volumeMap_.clear();
		Logger::Output("AudioManager : 終了処理が完了しました", Logger::Level::Engine);
	}

	void AudioManager::Load(const std::string& filePath, AudioType type, float volume) {
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
		typeMap_[key] = type;
		Logger::Output("AudioManager : 音声を読み込みました - キー: " + key + ", タイプ: " + AudioTypeToString(type) + ", パス: " + filePath, Logger::Level::Assets);
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

		float typeVolume = 1.0f;
		auto typeIt = typeMap_.find(lowerKey);
		if (typeIt != typeMap_.end()) {
			switch (typeIt->second) {
			case AudioType::BGM:   typeVolume = bgmVolume_;   break;
			case AudioType::SE:    typeVolume = seVolume_;    break;
			case AudioType::Voice: typeVolume = voiceVolume_; break;
			default: break;
			}
		}
		float volume = volumeMap_[lowerKey] * masterVolume_ * typeVolume;
		Logger::Output("AudioManager : 音声を再生します - キー: " + lowerKey + ", ループ: " + (loop ? "有効" : "無効") + ", 音量: " + std::to_string(volume), Logger::Level::Application);
		it->second->PlayAudio(volume, loop);
	}

	void AudioManager::Stop(const std::string& key) {
		// キーを小文字に変換して検索
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

		auto it = audioMap_.find(lowerKey);
		if (it != audioMap_.end() && it->second) {
			Logger::Output("AudioManager : 音声を停止します - キー: " + lowerKey, Logger::Level::Application);
			it->second->StopAll();
		} else {
			Logger::Output("AudioManager : 音声の停止に失敗しました - キーが見つかりません: " + key, Logger::Level::Warning);
		}
	}

	void AudioManager::StopAll() {
		Logger::Output("AudioManager : 全ての音声を停止します", Logger::Level::Application);
		for (auto& [key, audio] : audioMap_) {
			if (audio) {
				audio->StopAll();
			}
		}
	}

	void AudioManager::Update() {
		for (auto& [key, audio] : audioMap_) {
			if (audio) {
				float typeVolume = 1.0f;
				auto typeIt = typeMap_.find(key);
				if (typeIt != typeMap_.end()) {
					switch (typeIt->second) {
					case AudioType::BGM:   typeVolume = bgmVolume_;   break;
					case AudioType::SE:    typeVolume = seVolume_;    break;
					case AudioType::Voice: typeVolume = voiceVolume_; break;
					default: break;
					}
				}
				audio->SetVolume(volumeMap_[key] * masterVolume_ * typeVolume);
				audio->Update();
			}
		}
	}

	void AudioManager::SetMasterVolume(float volume) {
		Logger::Output("AudioManager : マスターボリュームを " + std::to_string(volume) + " に設定しました", Logger::Level::Application);
		masterVolume_ = volume;
	}

	void AudioManager::SetBGMVolume(float volume) {
		Logger::Output("AudioManager : BGM音量を " + std::to_string(volume) + " に設定しました", Logger::Level::Application);
		bgmVolume_ = volume;
	}

	void AudioManager::SetSEVolume(float volume) {
		Logger::Output("AudioManager : SE音量を " + std::to_string(volume) + " に設定しました", Logger::Level::Application);
		seVolume_ = volume;
	}

	void AudioManager::SetVoiceVolume(float volume) {
		Logger::Output("AudioManager : Voice音量を " + std::to_string(volume) + " に設定しました", Logger::Level::Application);
		voiceVolume_ = volume;
	}

	AudioType AudioManager::GetAudioType(const std::string& key) const {
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
		auto it = typeMap_.find(lowerKey);
		if (it != typeMap_.end()) {
			return it->second;
		}
		return AudioType::SE;
	}

	AudioType AudioManager::DetermineTypeFromPath(const std::filesystem::path& filePath) const {
		// ファイルパスの各ディレクトリ名をチェックし、SE / BGM / Voiceに一致するものを返す
		for (const auto& part : filePath) {
			const std::string name = part.string();
			if (name == "SE")    return AudioType::SE;
			if (name == "BGM")   return AudioType::BGM;
			if (name == "Voice") return AudioType::Voice;
		}
		return AudioType::SE;
	}

	void AudioManager::SetVolume(const std::string& key, float volume) {
		// キーを小文字に変換して検索
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

		auto it = volumeMap_.find(lowerKey);
		if (it != volumeMap_.end()) {
			Logger::Output("AudioManager : " + lowerKey + " の音量を " + std::to_string(volume) + " に設定しました", Logger::Level::Application);
			it->second = volume;
			// 即座に反映
			auto audioIt = audioMap_.find(lowerKey);
			if (audioIt != audioMap_.end() && audioIt->second) {
				// 最終的な音量を計算して適用する
				AudioType type = GetAudioType(lowerKey);
				float categoryVol = (type == AudioType::SE) ? seVolume_ :
					(type == AudioType::BGM) ? bgmVolume_ : voiceVolume_;

				audioIt->second->SetVolume(volume * categoryVol * masterVolume_);
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

	bool AudioManager::IsPlaying(const std::string& key) const {
		std::string lowerKey = key;
		std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
		auto it = audioMap_.find(lowerKey);

		if (it != audioMap_.end() && it->second) {
			return it->second->IsPlaying();
		}
		return false;
	}
} // namespace MadoEngine