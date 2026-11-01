#include "SceneBgmController.h"
#include "Audio/AudioManager.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>

namespace {
	constexpr const char* kSelectionBgmKey = "UpgradeBGM";

	// 選択待機用BGMの開始と終了にかける秒数
	constexpr float kSelectionBgmFadeInDuration = 0.3f;
	constexpr float kSelectionBgmFadeOutDuration = 0.5f;
}

void SceneBgmController::ChangeScene(SceneType sceneType) {

	// 同じ通常BGMを引き継ぐScene切り替えでも直前の選択待機用BGMを終了
	StopSelectionBgm();
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

void SceneBgmController::Update(float deltaTime, float transitionEffectProgress, bool isPaused, int selectionBgmPlaybackId) {
	MadoEngine::AudioManager& audioManager = MadoEngine::AudioManager::GetInstance();
	const float clampedProgress = std::clamp(transitionEffectProgress, 0.0f, 1.0f);

	// Scene遷移の減衰と選択用BGMのフェードを独立した係数として反映
	audioManager.SetBGMTransitionGain(1.0f - clampedProgress);
	UpdateSelectionBgm(deltaTime, selectionBgmPlaybackId);

	// 選択終了後もフェードアウト完了まで通常BGMの一時停止を維持
	const bool shouldPauseCurrentBgm = isPaused || selectionBgmState_ != SelectionBgmState::Stopped;
	if (!currentBgmKey_.empty() && isPaused_ != shouldPauseCurrentBgm) {
		audioManager.SetPaused(currentBgmKey_, shouldPauseCurrentBgm);
		isPaused_ = shouldPauseCurrentBgm;
	}
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
	if (bgmIt != bgmKeys_.end()) {
		return bgmIt->second;
	} else {
		return std::string{};
	}
}

void SceneBgmController::StopCurrentBgm() {
	StopSelectionBgm();

	// Scene切り替え後のBGMへ直前の一時停止状態を持ち越さないため初期化
	isPaused_ = false;
	if (currentBgmKey_.empty()) {
		return;
	}

	MadoEngine::AudioManager::GetInstance().Stop(currentBgmKey_);
	currentBgmKey_.clear();
}

void SceneBgmController::UpdateSelectionBgm(float deltaTime, int playbackId) {

	// 正の武器選択区間と負のレリック獲得区間を区別し、0だけを再生終了として処理
	if (selectionBgmPlaybackId_ != playbackId) {
		selectionBgmPlaybackId_ = playbackId;
		if (selectionBgmState_ == SelectionBgmState::Stopped) {
			StartSelectionBgm();
		} else if (selectionBgmState_ != SelectionBgmState::FadingOut) {

			// フェードイン途中の終了でも現在音量から減衰し、切り替え時の音量跳ねを防止
			selectionBgmState_ = SelectionBgmState::FadingOut;
			selectionBgmFadeElapsedTime_ = 0.0f;
			selectionBgmFadeStartGain_ = selectionBgmGain_;
		}
	}

	if (selectionBgmState_ == SelectionBgmState::Stopped || selectionBgmState_ == SelectionBgmState::Playing) {
		return;
	}
	selectionBgmFadeElapsedTime_ += (std::max)(0.0f, deltaTime);
	MadoEngine::AudioManager& audioManager = MadoEngine::AudioManager::GetInstance();

	if (selectionBgmState_ == SelectionBgmState::FadingIn) {

		// 経過秒数から音量を求め、Frame Rateに依存しない開始フェード
		selectionBgmGain_ = 1.0f;
		if (kSelectionBgmFadeInDuration > 0.0f) {
			selectionBgmGain_ = std::clamp(selectionBgmFadeElapsedTime_ / kSelectionBgmFadeInDuration, 0.0f, 1.0f);
		}
		audioManager.SetPlaybackGain(kSelectionBgmKey, selectionBgmGain_);
		if (selectionBgmGain_ >= 1.0f) {
			selectionBgmState_ = SelectionBgmState::Playing;
		}
		return;
	}

	// 終了要求時の音量から指定秒数で無音へ補間し、無音到達後だけVoiceを完全停止
	float fadeProgress = 1.0f;
	if (kSelectionBgmFadeOutDuration > 0.0f) {
		fadeProgress = std::clamp(selectionBgmFadeElapsedTime_ / kSelectionBgmFadeOutDuration, 0.0f, 1.0f);
	}
	selectionBgmGain_ = selectionBgmFadeStartGain_ * (1.0f - fadeProgress);
	audioManager.SetPlaybackGain(kSelectionBgmKey, selectionBgmGain_);
	if (fadeProgress >= 1.0f) {

		// フェード中に変わった最新の選択区間を引き継ぎ、連続レベルアップも先頭から再生
		const int nextPlaybackId = selectionBgmPlaybackId_;
		StopSelectionBgm();
		selectionBgmPlaybackId_ = nextPlaybackId;
		StartSelectionBgm();
	}
}

void SceneBgmController::StartSelectionBgm() {
	if (selectionBgmPlaybackId_ == 0) {
		return;
	}

	// 再生開始前に係数を0へ設定し、先頭Frameだけ設定音量で鳴る状態を防止
	MadoEngine::AudioManager& audioManager = MadoEngine::AudioManager::GetInstance();
	if (audioManager.IsLoaded(kSelectionBgmKey)) {
		selectionBgmState_ = SelectionBgmState::FadingIn;
		selectionBgmGain_ = 0.0f;
		selectionBgmFadeElapsedTime_ = 0.0f;
		if (kSelectionBgmFadeInDuration <= 0.0f) {
			selectionBgmState_ = SelectionBgmState::Playing;
			selectionBgmGain_ = 1.0f;
		}
		audioManager.SetPlaybackGain(kSelectionBgmKey, selectionBgmGain_);
		audioManager.Play(kSelectionBgmKey, true);
	}
}

void SceneBgmController::StopSelectionBgm() {

	// Scene終了時は即時破棄し、次回再生へフェード係数を持ち越さないよう復帰
	MadoEngine::AudioManager& audioManager = MadoEngine::AudioManager::GetInstance();
	if (selectionBgmState_ != SelectionBgmState::Stopped && audioManager.IsLoaded(kSelectionBgmKey)) {
		audioManager.Stop(kSelectionBgmKey);
		audioManager.SetPlaybackGain(kSelectionBgmKey, 1.0f);
	}
	selectionBgmPlaybackId_ = 0;
	selectionBgmState_ = SelectionBgmState::Stopped;
	selectionBgmGain_ = 0.0f;
	selectionBgmFadeElapsedTime_ = 0.0f;
	selectionBgmFadeStartGain_ = 0.0f;
}
