#include "InGameSession.h"
#include "Input/MyInput.h"
#include <algorithm>

namespace {
	constexpr float kGameStartDelayDuration = 1.0f; // 開始前条件の成立後に挟む待機時間（秒）
}

namespace System {

	void InGameSession::Initialize(float timeLimit) {
		currentPhase_ = InGamePhase::Starting;
		gameStartDelayTimer_.Reset();
		timeLimit_ = (std::max)(0.0f, timeLimit);
		executionTime_ = 0.0f;
		isTimeUp_ = false;
	}

	float InGameSession::Update(float deltaTime, bool canStart) {

		// 開始前条件の成立後も待機時間を確保し、Playingへの移行をSession内で管理
		switch (currentPhase_) {
		case InGamePhase::Starting:
			if (!canStart) {
				return 0.0f;
			}
			if (!gameStartDelayTimer_.IsActive()) {

				// 条件成立Frameの時間を待機へ持ち込まないよう開始だけを実行
				gameStartDelayTimer_.Start(kGameStartDelayDuration);
				return 0.0f;
			}
			gameStartDelayTimer_.Update(deltaTime);
			if (!gameStartDelayTimer_.IsFinished()) {
				return 0.0f;
			}
			currentPhase_ = InGamePhase::Playing;

			// 待機終了Frameの時間を制限時間とGameObjectへ重複加算しない開始境界
			return 0.0f;
		case InGamePhase::Playing:
			if (MyInput::Trigger("Pause")) {
				currentPhase_ = InGamePhase::Paused;
			}
			break;
		case InGamePhase::Paused:
			if (MyInput::Trigger("Pause")) {
				currentPhase_ = InGamePhase::Playing;
			}
			break;
		case InGamePhase::WaitingUpgrade:
		case InGamePhase::WaitingGetItem:
		case InGamePhase::GameOver:
			break;
		}

		if (!IsPlaying()) {
			return 0.0f;
		}

		// 制限時間とGameObjectへ同じ経過時間を適用して開始待機や停止との時間差を防止
		const float gameDeltaTime = (std::max)(0.0f, deltaTime);
		executionTime_ = (std::min)(timeLimit_, executionTime_ + gameDeltaTime);

		if (executionTime_ >= timeLimit_) {
			isTimeUp_ = true;

			// GameOverへの遷移判断は通知を受け取る呼び出し側へ委譲
		}
		return gameDeltaTime;
	}

	float InGameSession::GetStartDelayProgress() const {
		if (!gameStartDelayTimer_.IsActive() && !gameStartDelayTimer_.IsFinished()) {
			return 0.0f;
		}
		return gameStartDelayTimer_.GetProgress();
	}

	float InGameSession::GetRemainingTime() const {
		return (std::max)(0.0f, timeLimit_ - executionTime_);
	}

	void InGameSession::SetUpgradeSelectionActive(bool isActive) {

		// Upgrade選択による停止を通常プレイ中からの遷移に限定
		if (isActive && currentPhase_ == InGamePhase::Playing) {
			currentPhase_ = InGamePhase::WaitingUpgrade;
			return;
		}

		// 他の待機理由を誤って解除しないようWaitingUpgradeだけを再開
		if (!isActive && currentPhase_ == InGamePhase::WaitingUpgrade) {
			currentPhase_ = InGamePhase::Playing;
		}
	}

	void InGameSession::SetItemAcquisitionActive(bool isActive) {

		// 宝箱報酬を武器強化より優先して通常プレイから獲得待ちへ遷移
		if (isActive && currentPhase_ == InGamePhase::Playing) {
			currentPhase_ = InGamePhase::WaitingGetItem;
			return;
		}

		// 他の停止理由を解除しないようレリック獲得待ちだけを再開
		if (!isActive && currentPhase_ == InGamePhase::WaitingGetItem) {
			currentPhase_ = InGamePhase::Playing;
		}
	}

	void InGameSession::SetGameOver() {
		currentPhase_ = InGamePhase::GameOver;
	}

} // namespace System
