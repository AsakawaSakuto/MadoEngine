#pragma once
#include "Math/Vector3.h"
#include "imguiHeaders.h"

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

#ifdef USE_IMGUI

	void DrawImGui(const char* label) {
		ImGui::Begin(label);
		ImGui::DragFloat3("Center", &center.x, 0.01f);
		ImGui::DragFloat3("Min", &min.x, 0.01f);
		ImGui::DragFloat3("Max", &max.x, 0.01f);
		ImGui::Combo("Direction", reinterpret_cast<int*>(&direction), "PulsX\0MinusX\0PulsZ\0MinusZ\0");
		ImGui::End();
	}

#endif // USE_IMGUI
};
