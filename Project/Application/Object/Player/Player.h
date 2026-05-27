#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MathHeaders.h"

class Player {
public:

	void Initialize();

	void Update();

private:
	Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	
	Shape hitbox_;
};