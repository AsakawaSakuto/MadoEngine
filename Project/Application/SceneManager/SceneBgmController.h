#pragma once
#include "SceneType.h"
#include "Utility/Random.h"
#include <map>
#include <string>

/// @brief SceneごとのBGM再生と一時停止、遷移音量を管理
class SceneBgmController final {
public:
	/// @brief 現在Sceneに対応するBGMへ切り替え
	/// @param sceneType 切替後のScene種別
	void ChangeScene(SceneType sceneType);

	/// @brief SceneのBGM一時停止状態と選択待機用BGM、遷移音量を更新
	/// @param deltaTime 前フレームからの経過時間、秒
	/// @param transitionEffectProgress 0.0fから1.0fの遷移Effect進行度
	/// @param isPaused BGMを一時停止する場合はtrue
	/// @param selectionBgmPlaybackId 選択待機用BGMの再生区間の識別値、再生不要の場合は0
	void Update(float deltaTime, float transitionEffectProgress, bool isPaused, int selectionBgmPlaybackId);

	/// @brief 再生中BGMの停止と遷移音量の初期化
	void Finalize();

private:
	enum class SelectionBgmState {
		Stopped,
		FadingIn,
		Playing,
		FadingOut,
	};

	/// @brief Sceneに対応するBGMキーを選択
	/// @param sceneType 選曲対象のScene種別
	/// @return 選択したBGMキー、BGM未設定の場合は空文字列
	std::string SelectBgmKey(SceneType sceneType);

	/// @brief 現在のBGMを停止
	void StopCurrentBgm();

	/// @brief 選択待機用BGMの再生区間変更とフェードを更新
	/// @param deltaTime 前フレームからの経過時間、秒
	/// @param playbackId 再生区間の識別値、再生不要の場合は0
	void UpdateSelectionBgm(float deltaTime, int playbackId);

	/// @brief 選択待機用BGMを無音から先頭ループ再生
	void StartSelectionBgm();

	/// @brief 選択待機用BGMを完全停止して再生区間を初期化
	void StopSelectionBgm();

	std::map<SceneType, std::string> bgmKeys_ = {
		{ SceneType::Title, "TitleBGM" },
		{ SceneType::Result, "ResultBGM" },
	};
	Random bgmRandom_;
	std::string currentBgmKey_;
	bool isPaused_ = false;
	int selectionBgmPlaybackId_ = 0;
	SelectionBgmState selectionBgmState_ = SelectionBgmState::Stopped;
	float selectionBgmGain_ = 0.0f;
	float selectionBgmFadeElapsedTime_ = 0.0f;
	float selectionBgmFadeStartGain_ = 0.0f;
};
