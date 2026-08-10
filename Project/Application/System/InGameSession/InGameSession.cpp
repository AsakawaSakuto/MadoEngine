#include "InGameSession.h"
#include "Input/MyInput.h"
#include <algorithm>

namespace System {

	void InGameSession::Initialize(float timeLimit) {
		currentPhase_ = InGamePhase::Starting;
		timeLimit_ = (std::max)(0.0f, timeLimit);
		executionTime_ = 0.0f;
		isTimeUp_ = false;
	}

	void InGameSession::Update(float deltaTime) {

		// Pause入力による遷移をPlayingとPausedの間だけに限定
		switch (currentPhase_) {
		case InGamePhase::Starting:
			currentPhase_ = InGamePhase::Playing;
			break;
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
			return;
		}

		// Pauseや選択待機中に制限時間が進まないようPlaying中だけ加算
		executionTime_ = (std::min)(timeLimit_,executionTime_ + (std::max)(0.0f, deltaTime));

		if (executionTime_ >= timeLimit_) {
			isTimeUp_ = true;

			// GameOverへの遷移判断は通知を受け取る呼び出し側へ委譲
		}
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

} // namespace System
