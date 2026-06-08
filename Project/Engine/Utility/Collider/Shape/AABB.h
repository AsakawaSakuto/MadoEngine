#pragma once
#include "Math/Vector3.h"
#include "imguiHeaders.h"

struct AABB {
	Vector3 center = { 0.0f, 0.0f, 0.0f }; // 中心点
	Vector3 min = { -0.5f, -0.5f, -0.5f }; // 最小点（ローカルオフセット）
	Vector3 max = { 0.5f, 0.5f, 0.5f };    // 最大点（ローカルオフセット）

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
		ImGui::End();
	}

#endif // USE_IMGUI
};