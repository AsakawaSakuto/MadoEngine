#pragma once
#include "SceneType.h"
#include "Utility/Random.h"
#include <map>
#include <string>

/// @brief SceneごとのBGM再生と遷移音量を管理
class SceneBgmController final {
public:
	/// @brief 現在Sceneに対応するBGMへ切り替え
	/// @param sceneType 切替後のScene種別
	void ChangeScene(SceneType sceneType);

	/// @brief シーン遷移進行度をBGM音量へ反映
	/// @param transitionEffectProgress 0.0fから1.0fの遷移Effect進行度
	void Update(float transitionEffectProgress);

	/// @brief 再生中BGMの停止と遷移音量の初期化
	void Finalize();

private:
	/// @brief Sceneに対応するBGMキーを選択
	/// @param sceneType 選曲対象のScene種別
	/// @return 選択したBGMキー、BGM未設定の場合は空文字列
	std::string SelectBgmKey(SceneType sceneType);

	/// @brief 現在のBGMを停止
	void StopCurrentBgm();

	std::map<SceneType, std::string> bgmKeys_ = {
		{ SceneType::Title, "TitleBGM" },
		{ SceneType::Result, "ResultBGM" },
	};
	Random bgmRandom_;
	std::string currentBgmKey_;
};
