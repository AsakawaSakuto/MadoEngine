#pragma once
#pragma once
#include "AudioManager.h"

/// @brief オーディオ管理の簡易ラッパー名前空間
/// AudioManagerへの簡潔なアクセスを提供
namespace MyAudio {

	/// @brief 音声種別ごとに同じ音声の同時再生数上限を設定
	/// @param type 上限を設定する音声種別
	/// @param maxInstances 同じ音声の同時再生数上限、0の場合は新規再生を抑止
	inline void SetMaxInstancesPerSound(AudioType type, int maxInstances) {
		MadoEngine::AudioManager::GetInstance().SetMaxInstancesPerSound(type, maxInstances);
	}

	/// @brief 音声種別に対応する同じ音声の同時再生数上限を取得
	/// @param type 上限を取得する音声種別
	/// @return 同じ音声の同時再生数上限、未対応の種別の場合は0
	inline int GetMaxInstancesPerSound(AudioType type) {
		return MadoEngine::AudioManager::GetInstance().GetMaxInstancesPerSound(type);
	}

	/// @brief 音声を再生
	/// @param key ファイル名（拡張子なし）、例: "fire", "titleBGM"
	/// @param loop ループ再生するか（デフォルト: false）
	inline void Play(const std::string& key, bool loop = false) {
		MadoEngine::AudioManager::GetInstance().Play(key, loop);
	}

	/// @brief 特定の音声を停止
	/// @param key ファイル名（拡張子なし）
	inline void Stop(const std::string& key) {
		MadoEngine::AudioManager::GetInstance().Stop(key);
	}

	/// @brief 全ての音声を停止
	inline void StopAll() {
		MadoEngine::AudioManager::GetInstance().StopAll();
	}

	/// @brief 個別の音声の音量を設定
	/// @param key ファイル名（拡張子なし）
	/// @param volume 音量（0.0f～2.0f）
	inline void SetVolume(const std::string& key, float volume) {
		MadoEngine::AudioManager::GetInstance().SetVolume(key, volume);
	}

	/// @brief 個別の音声の音量を取得
	/// @param key ファイル名（拡張子なし）
	/// @return 音量（見つからない場合は1.0f）
	inline float GetVolume(const std::string& key) {
		return MadoEngine::AudioManager::GetInstance().GetVolume(key);
	}

	/// @brief 再生速度を維持した個別音声の音程を変更
	/// @param key ファイル名、拡張子なし
	/// @param semitones 元の音程からの半音数、-12から12の範囲
	/// @return 音程の適用に成功した場合はtrue
	inline bool SetPitch(const std::string& key, float semitones) {
		return MadoEngine::AudioManager::GetInstance().SetPitch(key, semitones);
	}

	/// @brief 個別音声の音程設定を取得
	/// @param key ファイル名、拡張子なし
	/// @return 元の音程からの半音数、未登録の場合は0
	inline float GetPitch(const std::string& key) {
		return MadoEngine::AudioManager::GetInstance().GetPitch(key);
	}

	/// @brief マスターボリュームを設定
	/// @param volume 音量（0.0f～1.0f）
	inline void SetMasterVolume(float volume) {
		MadoEngine::AudioManager::GetInstance().SetMasterVolume(volume);
	}

	/// @brief マスターボリュームを取得
	/// @return 現在のマスターボリューム
	inline float GetMasterVolume() {
		return MadoEngine::AudioManager::GetInstance().GetMasterVolume();
	}

	/// @brief 音声が読み込み済みか確認
	/// @param key ファイル名（拡張子なし）
	/// @return 読み込み済みならtrue
	inline bool IsLoaded(const std::string& key) {
		return MadoEngine::AudioManager::GetInstance().IsLoaded(key);
	}
}
