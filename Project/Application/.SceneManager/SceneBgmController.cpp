#include "SceneBgmController.h"
#include "Audio/AudioManager.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>

void SceneBgmController::ChangeScene(SceneType sceneType) {
	MadoEngine::AudioManager& audioManager = MadoEngine::AudioManager::GetInstance();
	const std::string nextBgmKey = SelectBgmKey(sceneType);
	if (nextBgmKey.empty()) {

		// BGM未設定Sceneへ移った場合も直前SceneのBGMを残さないため停止
		StopCurrentBgm();
		return;
	}

	if (currentBgmKey_ == nextBgmKey && audioManager.IsPlaying(currentBgmKey_)) {
		return;
	}

	// 複数BGMの重複再生を避けるため次のBGM開始前に現在のBGMを停止
	StopCurrentBgm();
	if (!audioManager.IsLoaded(nextBgmKey)) {
		Logger::Output(
			"SceneBgmController : BGMが読み込まれていません - キー: " + nextBgmKey,
			Logger::Level::Warning
		);
		return;
	}

	currentBgmKey_ = nextBgmKey;
	audioManager.Play(currentBgmKey_, true);
}

void SceneBgmController::Update(float transitionEffectProgress) {
	const float clampedProgress = std::clamp(transitionEffectProgress, 0.0f, 1.0f);

	// 退出時の減衰と進入時の復帰を同じEffect進行度から生成
	MadoEngine::AudioManager::GetInstance().SetBGMTransitionGain(1.0f - clampedProgress);
}

void SceneBgmController::Finalize() {
	StopCurrentBgm();
	MadoEngine::AudioManager::GetInstance().SetBGMTransitionGain(1.0f);
}

std::string SceneBgmController::SelectBgmKey(SceneType sceneType) {
	if (sceneType == SceneType::Game) {

		// Game進行用の乱数列へ影響させず入場ごとにBGM1からBGM5を均等抽選
		return "BGM" + std::to_string(bgmRandom_.Int(1, 5));
	}

	const auto bgmIt = bgmKeys_.find(sceneType);
	return bgmIt != bgmKeys_.end() ? bgmIt->second : std::string{};
}

void SceneBgmController::StopCurrentBgm() {
	if (currentBgmKey_.empty()) {
		return;
	}

	MadoEngine::AudioManager::GetInstance().Stop(currentBgmKey_);
	currentBgmKey_.clear();
}
