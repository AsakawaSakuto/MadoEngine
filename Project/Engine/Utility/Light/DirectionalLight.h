#pragma once
#include"Math/Vector3.h"
#include"Math/Vector4.h"

struct DirectionalLight {
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector3 direction = { 0.0f, -1.0f, 0.0f };
	float pad1;
	float intensity = 0.6f;
	uint32_t useLight = 1;
	uint32_t useHalfLambert = 0;
	float pad2;
};
