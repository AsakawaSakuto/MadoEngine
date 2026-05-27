#pragma once
#include "Math/Vector3.h"
#include "string"

// 球体
struct Sphere {
	Vector3 center = { 0.0f,0.0f,0.0f }; // 中心点
	float radius = 1.0f;                 // 半径

	void DrawImGui(std::string name) {
		ImGui::Begin(name.c_str());
		ImGui::DragFloat3("Sphere Center", &center.x, 0.01f);
		ImGui::DragFloat("Sphere Radius", &radius, 0.01f);
		ImGui::End();
	}
};