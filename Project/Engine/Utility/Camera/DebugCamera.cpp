#include "Utility/Camera/DebugCamera.h"
#include "Input/MyInput.h"
#include "imguiHeaders.h"
#include <algorithm>
#include <corecrt_math_defines.h>

DebugCamera::DebugCamera() {
	ApplySphericalCoord();
	Camera::Update(1.0f / 60.0f);
}

void DebugCamera::Update(float deltaTime) {
	auto* mouse = MyInput::GetMouse();
	if (!mouse) {
		Camera::Update(deltaTime);
		return;
	}

	Vector2 delta = mouse->GetDelta();

	// ホイール中ボタン長押しドラッグ
	if (mouse->IsPress(MOUSE_M)) {
		auto* keybord = MyInput::GetKeybord();
		bool isShift = keybord && (keybord->IsPress(DIK_LSHIFT) || keybord->IsPress(DIK_RSHIFT));

		if (isShift) {
			// Shift + 中ドラッグ: カメラの向きに対して上下左右移動（パン）
			// 水平方向: yaw_ から右ベクトルを算出
			float cosY = std::cos(yaw_);
			float sinY = std::sin(yaw_);
			Vector3 right = { cosY, 0.0f, -sinY };
			Vector3 up    = { 0.0f, 1.0f,  0.0f };

			float panX = delta.x * panSensitivity_ * distance_ * deltaTime;
			float panY = delta.y * panSensitivity_ * distance_ * deltaTime;

			target_.x += right.x * panX + up.x * panY;
			target_.y += right.y * panX + up.y * panY;
			target_.z += right.z * panX + up.z * panY;
		} else {
			// 中ドラッグ: 注視点を中心にオービット回転
				yaw_   += delta.x * rotateSensitivity_ * deltaTime;
				pitch_ += delta.y * rotateSensitivity_ * deltaTime;

			// ピッチ角をクランプ（真上・真下を超えないように）
			constexpr float kPitchLimit = static_cast<float>(M_PI) * 0.499f;
			pitch_ = std::clamp(pitch_, -kPitchLimit, kPitchLimit);
		}
	}

	// マウスホイール: カメラの向いているベクトルに対して前後移動
	float wheel = mouse->GetWheelDelta();
	if (wheel != 0.0f) {
		distance_ -= wheel * dollySensitivity_ * distance_ * 0.1f * deltaTime;
		constexpr float kMinDistance = 1.0f;
		constexpr float kMaxDistance = 1000.0f;
		distance_ = std::clamp(distance_, kMinDistance, kMaxDistance);
	}

	ApplySphericalCoord();
	Camera::Update(deltaTime);
}

void DebugCamera::ApplySphericalCoord() {
	// 球面座標 → デカルト座標でカメラ位置を計算
	float cosP = std::cos(pitch_);
	float sinP = std::sin(pitch_);
	float cosY = std::cos(yaw_);
	float sinY = std::sin(yaw_);

	position_.x = target_.x + distance_ * cosP * sinY;
	position_.y = target_.y + distance_ * sinP;
	position_.z = target_.z + distance_ * cosP * cosY;

	// カメラ → ターゲットの前方ベクトルを計算
	Vector3 forward = {
		target_.x - position_.x,
		target_.y - position_.y,
		target_.z - position_.z
	};

	// 正規化
	float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
	if (len > 0.0f) {
		forward.x /= len;
		forward.y /= len;
		forward.z /= len;
	}

	// 前方ベクトルからピッチ・ヨーを導出（回転行列の実装依存を回避）
	rotation_.x = std::asin(-forward.y);
	rotation_.y = std::atan2(forward.x, forward.z);
	rotation_.z = 0.0f;
}

void DebugCamera::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Debug Camera");
	ImGui::DragFloat3("Target", &target_.x, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat("Distance", &distance_, 0.1f, 1.0f, 100.0f);
	ImGui::DragFloat("回転感度", &rotateSensitivity_, 0.01f);
	ImGui::DragFloat("パン感度", &panSensitivity_, 0.01f);
	ImGui::DragFloat("ドリー感度", &dollySensitivity_, 0.01f);
	ImGui::End();
#endif
}