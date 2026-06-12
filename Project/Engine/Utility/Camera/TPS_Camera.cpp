#include "Utility/Camera/TPS_Camera.h"
#include "Input/MyInput.h"
#include "imguiHeaders.h"

TPS_Camera::TPS_Camera() {
	currentTarget_ = targetPosition_;
	ApplySphericalCoord();
	Camera::Update();
}

void TPS_Camera::Update() {
	Update(kDefaultDeltaTime);
}

void TPS_Camera::Update(float deltaTime) {
	HandleInput(deltaTime);
	UpdateCurrentTarget();
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

	ClampPitch();
}

void TPS_Camera::UpdateCurrentTarget() {
	float t = std::clamp(followStrength_, kMinFollowStrength, kMaxFollowStrength);
	currentTarget_ += (targetPosition_ - currentTarget_) * t;
}

void TPS_Camera::ApplySphericalCoord() {
	Vector3 viewCenter = CalculateViewCenter();

	position_ = CalculateBasePosition(viewCenter);
	ApplyLookAtRotation(viewCenter);
}

Vector3 TPS_Camera::CalculateViewCenter() const {
	Vector3 forward = CalculateForwardDirection();
	Vector3 right = CalculateRightDirection();
	Vector3 up = Math::Cross(right, forward);

	// Offsetはカメラ位置ではなく、Targetからずらした注視中心として扱う
	return currentTarget_ + right * offset_.x + up * offset_.y + forward * offset_.z;
}

Vector3 TPS_Camera::CalculateBasePosition(const Vector3& viewCenter) const {
	Vector3 forward = CalculateForwardDirection();

	// 注視中心から一定距離だけ後ろにカメラを配置する
	return viewCenter - forward * distance_;
}

Vector3 TPS_Camera::CalculateForwardDirection() const {
	float cosP = std::cos(pitch_);
	float sinP = std::sin(pitch_);
	float sinY = std::sin(yaw_);
	float cosY = std::cos(yaw_);

	return {
		-cosP * sinY,
		-sinP,
		-cosP * cosY,
	};
}

Vector3 TPS_Camera::CalculateRightDirection() const {
	float sinY = std::sin(yaw_);
	float cosY = std::cos(yaw_);

	return { cosY, 0.0f, -sinY };
}

void TPS_Camera::ApplyLookAtRotation(const Vector3& viewCenter) {
	Vector3 lookDir = (viewCenter - position_).Normalized();

	rotation_.x = std::asin(-lookDir.y);
	rotation_.y = std::atan2(lookDir.x, lookDir.z);
	rotation_.z = 0.0f;
}

void TPS_Camera::ClampPitch() {
	pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);
}

void TPS_Camera::DrawImGui() {

#ifdef USE_IMGUI

	ImGui::Begin("TPS Camera");

	ImGui::Text("Target: (%.2f, %.2f, %.2f)", targetPosition_.x, targetPosition_.y, targetPosition_.z);
	ImGui::SameLine();
	ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", position_.x, position_.y, position_.z);

	ImGui::DragFloat3("Current Target (smoothed)", &currentTarget_.x, 0.1f);
	ImGui::Separator();

	float yawDeg   = yaw_   * kRadiansToDegrees;
	float pitchDeg = pitch_ * kRadiansToDegrees;
	ImGui::SliderFloat("Yaw (deg)",   &yawDeg,   -180.0f, 180.0f);
	ImGui::SliderFloat("Pitch (deg)", &pitchDeg,
		minPitch_ * kRadiansToDegrees,
		maxPitch_ * kRadiansToDegrees);
	yaw_   = yawDeg   * kDegreesToRadians;
	pitch_ = pitchDeg * kDegreesToRadians;
	ClampPitch();

	ImGui::DragFloat("Distance",          &distance_,          0.1f, 1.0f, 100.0f);
	ImGui::DragFloat("Min Pitch (deg)", &minPitch_, 1.0f, -89.0f, 89.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragFloat("Max Pitch (deg)", &maxPitch_, 1.0f, -89.0f, 89.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
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
