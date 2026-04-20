#include "AudioManager.h"
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
		Logger::Info("AudioManager : 初期化を開始しました");

		// マップのクリア
		audioMap_.clear();
		volumeMap_.clear();

		// Assets/Audio ディレクトリ内の全音声ファイルを自動ロード
		std::filesystem::path audioDir(kAudioDirectory);
		if (std::filesystem::exists(audioDir) && std::filesystem::is_directory(audioDir)) {
			Logger::Info("AudioManager : " + std::string(kAudioDirectory) + " から音声ファイルを読み込んでいます");
			LoadDirectory(audioDir);
			Logger::Info("AudioManager : " + std::to_string(audioMap_.size()) + " 個の音声ファイルを読み込みました");
		} else {
			Logger::Warning("AudioManager : 音声ディレクトリが見つかりません: " + std::string(kAudioDirectory));
		}

		Logger::Info("AudioManager : 初期化が完了しました");
	}

	void AudioManager::LoadDirectory(const std::filesystem::path& directory) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
			if (!entry.is_regular_file()) continue;

			auto ext = entry.path().extension().string();
			// 小文字に変換して比較
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".wav" || ext == ".mp3") {
				Logger::Debug("AudioManager : ファイルを読み込んでいます: " + entry.path().string());
				Load(entry.path().string(), 1.0f);
			}
		}
	}

	std::string AudioManager::MakeKey(const std::filesystem::path& filePath) const {
		// ファイル名から拡張子を除いたものをキーとする
		return filePath.stem().string();
	}

	void AudioManager::Finalize() {
		Logger::Info("AudioManager : 終了処理を開始しました");
		StopAll();
		audioMap_.clear();
		volumeMap_.clear();
		Logger::Info("AudioManager : 終了処理が完了しました");
	}

	void AudioManager::Load(const std::string& filePath, float volume) {
		std::filesystem::path path(filePath);
		std::string key = MakeKey(path);

		// 既に読み込み済みならスキップ
		if (audioMap_.find(key) != audioMap_.end()) {
			Logger::Warning("AudioManager : 既に読み込み済みのため、スキップします: " + key);
			return;
		}

		auto audio = std::make_unique<AudioX>();
		audio->Initialize(filePath);
		audioMap_[key] = std::move(audio);
		volumeMap_[key] = volume;
		Logger::Debug("AudioManager : 音声を読み込みました - キー: " + key + ", パス: " + filePath);
	}

	void AudioManager::Play(const std::string& key, bool loop) {
		auto it = audioMap_.find(key);
		if (it == audioMap_.end()) {
			Logger::Error("AudioManager : 音声の再生に失敗しました - キーが見つかりません: " + key);
			assert(false && "Audio not loaded");
			return;
		}

		float volume = volumeMap_[key] * masterVolume_;
		Logger::Debug("AudioManager : 音声を再生します - キー: " + key + ", ループ: " + (loop ? "有効" : "無効") + ", 音量: " + std::to_string(volume));
		it->second->PlayAudio(volume, loop);
	}

	void AudioManager::Stop(const std::string& key) {
		auto it = audioMap_.find(key);
		if (it != audioMap_.end() && it->second) {
			Logger::Debug("AudioManager : 音声を停止します - キー: " + key);
			it->second->StopAll();
		} else {
			Logger::Warning("AudioManager : 音声の停止に失敗しました - キーが見つかりません: " + key);
		}
	}

	void AudioManager::StopAll() {
		Logger::Debug("AudioManager : 全ての音声を停止します");
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
		Logger::Debug("AudioManager : マスターボリュームを " + std::to_string(volume) + " に設定しました");
		masterVolume_ = volume;
	}

	void AudioManager::SetVolume(const std::string& key, float volume) {
		auto it = volumeMap_.find(key);
		if (it != volumeMap_.end()) {
			Logger::Debug("AudioManager : " + key + " の音量を " + std::to_string(volume) + " に設定しました");
			it->second = volume;
			// 即座に反映
			auto audioIt = audioMap_.find(key);
			if (audioIt != audioMap_.end() && audioIt->second) {
				audioIt->second->SetVolume(volume * masterVolume_);
			}
		} else {
			Logger::Warning("AudioManager	: 音量の設定に失敗しました - キーが見つかりません: " + key);
		}
	}

	float AudioManager::GetVolume(const std::string& key) const {
		auto it = volumeMap_.find(key);
		if (it != volumeMap_.end()) {
			return it->second;
		}
		return 1.0f;
	}

	bool AudioManager::IsLoaded(const std::string& key) const {
		return audioMap_.find(key) != audioMap_.end();
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