#include "Utility/Camera/TPS_Camera.h"
#include "Input/MyInput.h"
#include "imguiHeaders.h"
#include <algorithm>
#include <cmath>
#include <corecrt_math_defines.h>

TPS_Camera::TPS_Camera() {
	currentTarget_ = targetPosition_;
	ApplySphericalCoord();
	Camera::Update();
}

void TPS_Camera::Update() {
	Update(1.0f / 60.0f);
}

void TPS_Camera::Update(float deltaTime) {
	HandleInput(deltaTime);

	// --- 追従対象の取得 ---
	Vector3 desiredTarget = targetPosition_;

	// --- スムージング追従（Lerp） ---
	float t = std::clamp(followStrength_, 0.0f, 1.0f);
	currentTarget_.x += (desiredTarget.x - currentTarget_.x) * t;
	currentTarget_.y += (desiredTarget.y - currentTarget_.y) * t;
	currentTarget_.z += (desiredTarget.z - currentTarget_.z) * t;

	ApplySphericalCoord();
	Camera::Update();
}

void TPS_Camera::HandleInput(float deltaTime) {
	// --- マウス入力 ---
	if (useMouseInput_) {
		auto* mouse = MyInput::GetMouse();
		if (mouse) {
			Vector2 delta = mouse->GetDelta();
			yaw_   += delta.x * mouseSensitivity_;
			pitch_ += delta.y * mouseSensitivity_;
		}
	}

	// --- ゲームパッド右スティック入力 ---
	if (useGamePadInput_) {
		auto* pad = MyInput::GetGamePad();
		if (pad && pad->IsConnected()) {
			Vector2 stick = pad->GetRightStick();
			yaw_   += stick.x * gamePadSensitivity_ * deltaTime;
			pitch_ -= stick.y * gamePadSensitivity_ * deltaTime; // スティック上でカメラ上方向
		}
	}

	// Pitch クランプ
	pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);
}

void TPS_Camera::ApplySphericalCoord() {
	float cosP = std::cos(pitch_);
	float sinP = std::sin(pitch_);
	float cosY = std::cos(yaw_);
	float sinY = std::sin(yaw_);

	// 球面座標からカメラの基本位置を算出
	Vector3 basePos;
	basePos.x = currentTarget_.x + distance_ * cosP * sinY;
	basePos.y = currentTarget_.y + distance_ * sinP;
	basePos.z = currentTarget_.z + distance_ * cosP * cosY;

	// --- オフセット適用（ターゲット座標系の右・上ベクトルで変換）---
	// 右ベクトル（Yaw から水平右方向）
	Vector3 right = { cosY, 0.0f, -sinY };

	// 前方ベクトル（ターゲット → カメラの逆）
	Vector3 forward = {
		currentTarget_.x - basePos.x,
		currentTarget_.y - basePos.y,
		currentTarget_.z - basePos.z,
	};
	float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
	if (len > 0.0f) {
		forward.x /= len;
		forward.y /= len;
		forward.z /= len;
	}

	// 上ベクトル（right × forward の外積）
	Vector3 up = {
		right.y * forward.z - right.z * forward.y,
		right.z * forward.x - right.x * forward.z,
		right.x * forward.y - right.y * forward.x,
	};

	position_.x = basePos.x + right.x * offset_.x + up.x * offset_.y + forward.x * offset_.z;
	position_.y = basePos.y + right.y * offset_.x + up.y * offset_.y + forward.y * offset_.z;
	position_.z = basePos.z + right.z * offset_.x + up.z * offset_.y + forward.z * offset_.z;

	// 回転を前方ベクトルから導出
	Vector3 lookDir = {
		currentTarget_.x - position_.x,
		currentTarget_.y - position_.y,
		currentTarget_.z - position_.z,
	};
	float lookLen = std::sqrt(lookDir.x * lookDir.x + lookDir.y * lookDir.y + lookDir.z * lookDir.z);
	if (lookLen > 0.0f) {
		lookDir.x /= lookLen;
		lookDir.y /= lookLen;
		lookDir.z /= lookLen;
	}

	rotation_.x = std::asin(-lookDir.y);
	rotation_.y = std::atan2(lookDir.x, lookDir.z);
	rotation_.z = 0.0f;
}

void TPS_Camera::SetFollowStrength(float strength) {
	followStrength_ = std::clamp(strength, 0.0f, 1.0f);
}

void TPS_Camera::DrawImGui() {

#ifdef USE_IMGUI

	ImGui::Begin("TPS Camera");

	ImGui::Text("Target: (%.2f, %.2f, %.2f)", targetPosition_.x, targetPosition_.y, targetPosition_.z);
	ImGui::SameLine();
	ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", position_.x, position_.y, position_.z);

	ImGui::DragFloat3("Current Target (smoothed)", &currentTarget_.x, 0.1f);
	ImGui::Separator();

	float yawDeg   = yaw_   * (180.0f / static_cast<float>(M_PI));
	float pitchDeg = pitch_ * (180.0f / static_cast<float>(M_PI));
	ImGui::SliderFloat("Yaw (deg)",   &yawDeg,   -180.0f, 180.0f);
	ImGui::SliderFloat("Pitch (deg)", &pitchDeg,
		minPitch_ * (180.0f / static_cast<float>(M_PI)),
		maxPitch_ * (180.0f / static_cast<float>(M_PI)));
	yaw_   = yawDeg   * (static_cast<float>(M_PI) / 180.0f);
	pitch_ = pitchDeg * (static_cast<float>(M_PI) / 180.0f);

	ImGui::DragFloat("Distance",          &distance_,          0.1f, 1.0f, 100.0f);
	ImGui::DragFloat3("Offset",           &offset_.x,          0.01f);
	ImGui::SliderFloat("Follow Strength", &followStrength_,    0.0f, 1.0f);
	ImGui::Separator();
	ImGui::Checkbox("Use Mouse Input",   &useMouseInput_);
	ImGui::SameLine();
	ImGui::Checkbox("Use GamePad Input", &useGamePadInput_);
	ImGui::DragFloat("Mouse Sensitivity", &mouseSensitivity_,  0.0001f, 0.0001f, 0.05f);
	ImGui::DragFloat("Pad Sensitivity",   &gamePadSensitivity_,0.1f,   0.1f,    10.0f);
	ImGui::End();

#endif
}
