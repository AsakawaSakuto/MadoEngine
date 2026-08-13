#include "SceneTransitionController.h"
#include <algorithm>

bool SceneTransitionController::Request(SceneType destinationSceneType) {
	if (state_ != State::Idle || destinationSceneType == SceneType::None) {
		return false;
	}

	// 遷移元の演出完了後も遷移先を維持できるよう要求時点で固定
	destinationSceneType_ = destinationSceneType;
	timer_.Start(config_.exitDuration);
	state_ = State::PlayingExit;
	return true;
}

void SceneTransitionController::Update(float deltaTime) {
	if (state_ != State::PlayingExit && state_ != State::PlayingEnter) {
		return;
	}

	timer_.Update(deltaTime);
	if (!timer_.IsFinished()) {
		return;
	}

	if (state_ == State::PlayingExit) {

		// Scene破棄をEffect最大Frameの描画後まで遅延するため切替待機へ遷移
		state_ = State::WaitingForSceneChange;
		return;
	}

	// 遷移先の演出完了時点で次の遷移要求を受け付けられる待機状態へ復帰
	timer_.Reset();
	destinationSceneType_ = SceneType::None;
	state_ = State::Idle;
}

bool SceneTransitionController::NotifySceneChanged(SceneType currentSceneType) {
	if (state_ != State::WaitingForSceneChange || currentSceneType != destinationSceneType_) {
		return false;
	}

	// 新Sceneを最大Effect強度から表示するため切替完了後に遷移先の演出を開始
	timer_.Start(config_.enterDuration);
	state_ = State::PlayingEnter;
	return true;
}

void SceneTransitionController::Reset() {
	timer_.Reset();
	destinationSceneType_ = SceneType::None;
	state_ = State::Idle;
}

bool SceneTransitionController::IsTransitioning() const {
	return state_ != State::Idle;
}

bool SceneTransitionController::IsSceneChangeReady() const {
	return state_ == State::WaitingForSceneChange;
}

SceneType SceneTransitionController::GetDestinationSceneType() const {
	return destinationSceneType_;
}

float SceneTransitionController::GetProgress() const {
	if (state_ == State::Idle) {
		return 0.0f;
	}
	if (state_ == State::WaitingForSceneChange) {
		return 1.0f;
	}
	return timer_.GetProgress();
}

float SceneTransitionController::GetEffectProgress() const {
	switch (state_) {
	case State::PlayingExit:
		return timer_.GetProgress();
	case State::WaitingForSceneChange:
		return 1.0f;
	case State::PlayingEnter:
		return timer_.GetReverseProgress();
	case State::Idle:
	default:
		return 0.0f;
	}
}

void SceneTransitionController::SetConfig(const SceneTransitionConfig& config) {
	config_.exitDuration = (std::max)(0.0f, config.exitDuration);
	config_.enterDuration = (std::max)(0.0f, config.enterDuration);
}

const SceneTransitionConfig& SceneTransitionController::GetConfig() const {
	return config_;
}
