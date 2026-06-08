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
	float bottomExtendY = 0.0f; // 斜面角度を変えずに底面だけ下へ伸ばす量
	SlopeDirection direction = SlopeDirection::PulsX;

	/// @brief 底面延長を含む最小ワールド座標を取得する
	/// @return 底面延長後の最小ワールド座標
	Vector3 GetMinWorld() const {
		Vector3 worldMin = center + min;
		worldMin.y -= bottomExtendY > 0.0f ? bottomExtendY : 0.0f;
		return worldMin;
	}

	/// @brief 最大ワールド座標を取得する
	/// @return 最大ワールド座標
	Vector3 GetMaxWorld() const {
		return center + max;
	}

	/// @brief 斜面の低い端に使う最小ワールド座標を取得する
	/// @return 底面延長を含まない最小ワールド座標
	Vector3 GetSurfaceMinWorld() const {
		return center + min;
	}

#ifdef USE_IMGUI

	void DrawImGui(const char* label) {
		ImGui::Begin(label);
		ImGui::DragFloat3("Center", &center.x, 0.01f);
		ImGui::DragFloat3("Min", &min.x, 0.01f);
		ImGui::DragFloat3("Max", &max.x, 0.01f);
		ImGui::DragFloat("Bottom Extend Y", &bottomExtendY, 0.01f, 0.0f);
		ImGui::Combo("Direction", reinterpret_cast<int*>(&direction), "PulsX\0MinusX\0PulsZ\0MinusZ\0");
		ImGui::End();
	}

#endif // USE_IMGUI
};
