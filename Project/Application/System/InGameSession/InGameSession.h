#pragma once
#include "InGamePhase.h"
#include "UtilityHeaders.h"

class InGameSession {
public:
	
	void Initialize();

	void Update(float deltaTime);

	InGamePhase GetCurrentPhase() const { return currentPhase_; }
private:
	InGamePhase currentPhase_;
	float executionTime_;
};