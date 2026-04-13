#pragma once
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

struct Transform3D {
	Vector3 scale = { 1.0f,1.0f,1.0f };
	Vector3 rotate = { 0.0f,0.0f,0.0f };
	Vector3 translate = { 0.0f,0.0f,0.0f };

	void SetAllScale(float s) {
		scale = { s,s,s };
	}
};

struct Transform2D {
	Vector2 scale = { 1.0f,1.0f };
	float rotate = 0.0f;
	Vector2 translate = { 0.0f,0.0f };
};