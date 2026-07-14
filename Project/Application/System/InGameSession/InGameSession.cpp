#include "InGameSession.h"
#include "Input/MyInput.h"
#include <algorithm>

namespace {
	constexpr const char* kPauseAction = "Pause";
}

void InGameSession::Initialize() {
	currentPhase_ = InGamePhase::Starting;
	executionTime_ = 0.0f;
}

void InGameSession::Update(float deltaTime) {
	switch (currentPhase_) {
	case InGamePhase::Starting:
		currentPhase_ = InGamePhase::Playing;
		break;
	case InGamePhase::Playing:
		if (MyInput::Trigger(kPauseAction)) {
			currentPhase_ = InGamePhase::Paused;
		}
		break;
	case InGamePhase::Paused:
		if (MyInput::Trigger(kPauseAction)) {
			currentPhase_ = InGamePhase::Playing;
		}
		break;
	case InGamePhase::WaitingUpgrade:
	case InGamePhase::WaitingGetItem:
	case InGamePhase::GameOver:
	case InGamePhase::Count:
		break;
	}

	if (IsPlaying()) {
		executionTime_ += (std::max)(0.0f, deltaTime);
	}
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
