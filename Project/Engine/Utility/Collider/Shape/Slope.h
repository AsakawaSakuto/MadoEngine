#pragma once
#include "Math/Vector3.h"

enum class SlopeDirection {
	PulsX,
	MinusX,
	PulsZ,
	MinusZ
};

struct Slope {
	Vector3 center = { 0.0f, 0.0f, 0.0f };
	Vector3 min = { -0.5f, 0.0f, -0.5f };
	Vector3 max = { 0.5f, 0.5f, 0.5f };
	SlopeDirection direction = SlopeDirection::PulsX;

	Vector3 GetMinWorld() const {
		return center + min;
	}

	Vector3 GetMaxWorld() const {
		return center + max;
	}
};
