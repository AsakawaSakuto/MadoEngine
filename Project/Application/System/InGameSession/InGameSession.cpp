#include "InGameSession.h"

void InGameSession::Initialize() {
	currentPhase_ = InGamePhase::Starting;
	executionTime_ = 0.0f;
}

void InGameSession::Update(float deltaTime) {

	if (currentPhase_ == InGamePhase::Starting) {
		currentPhase_ = InGamePhase::Playing;
	}

	if (currentPhase_ == InGamePhase::Playing) {

		executionTime_ += deltaTime;

		if (MyInput::Trigger("Pause")) {
			currentPhase_ = InGamePhase::Paused;
		}
	}

	if (currentPhase_ == InGamePhase::Paused) {

		if (MyInput::Trigger("Pause")) {
			currentPhase_ = InGamePhase::Playing;
		}
	}
}