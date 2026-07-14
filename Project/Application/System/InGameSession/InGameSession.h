#pragma once
#include "InGamePhase.h"

class InGameSession {
public:
	
	void Initialize();

	void Update();

private:
	InGamePhase currentPhase;
};