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
		case InGamePhase::Count:
			break;
		}

		if (!IsPlaying()) {
			return;
		}

		executionTime_ = (std::min)(timeLimit_,executionTime_ + (std::max)(0.0f, deltaTime));

		if (executionTime_ >= timeLimit_) {
			isTimeUp_ = true;
			//currentPhase_ = InGamePhase::GameOver;
		}
	}

	float InGameSession::GetRemainingTime() const {
		return (std::max)(0.0f, timeLimit_ - executionTime_);
	}

	void InGameSession::SetUpgradeSelectionActive(bool isActive) {
		if (isActive && currentPhase_ == InGamePhase::Playing) {
			currentPhase_ = InGamePhase::WaitingUpgrade;
			return;
		}

		if (!isActive && currentPhase_ == InGamePhase::WaitingUpgrade) {
			currentPhase_ = InGamePhase::Playing;
		}
	}

} // namespace System