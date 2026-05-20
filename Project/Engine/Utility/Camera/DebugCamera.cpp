#include "Utility/Camera/DebugCamera.h"
#include "Input/MyInput.h"
#include <algorithm>
#include <corecrt_math_defines.h>

DebugCamera::DebugCamera() {
	ApplySphericalCoord();
	Camera::Update();
}

void DebugCamera::Update() {
	auto* mouse = Input::GetMouse();
	if (!mouse) {
		Camera::Update();
		return;
	}

	Vector2 delta = mouse->GetDelta();

	// 右ドラッグ: オービット回転
	if (mouse->IsPress(MOUSE_R)) {
		yaw_ += delta.x * rotateSensitivity_;
		pitch_ += delta.y * rotateSensitivity_;

		// ピッチ角をクランプ（真上・真下を超えないように）
		constexpr float kPitchLimit = static_cast<float>(M_PI) * 0.499f;
		pitch_ = std::clamp(pitch_, -kPitchLimit, kPitchLimit);
	}

	// 中ドラッグ: パン
	if (mouse->IsPress(MOUSE_M)) {
		// カメラのローカル右・上ベクトルをターゲットのオフセットに加算
		float cosP = std::cos(pitch_);
		float sinP = std::sin(pitch_);
		float cosY = std::cos(yaw_);
		float sinY = std::sin(yaw_);

		// 右ベクトル（Y成分は常に0）
		Vector3 right = { cosY, 0.0f, -sinY };
		// 上ベクトル
		Vector3 up = { sinY * sinP, cosP, cosY * sinP };

		float panX = -delta.x * panSensitivity_ * distance_ * 0.1f;
		float panY = delta.y * panSensitivity_ * distance_ * 0.1f;

		target_.x += right.x * panX + up.x * panY;
		target_.y += right.y * panX + up.y * panY;
		target_.z += right.z * panX + up.z * panY;
	}

	// ホイール: ズーム
	float wheel = mouse->GetWheelDelta();
	if (wheel != 0.0f) {
		distance_ -= wheel * zoomSensitivity_ * 0.01f;
		constexpr float kMinDistance = 0.5f;
		if (distance_ < kMinDistance) {
			distance_ = kMinDistance;
		}
	}

	ApplySphericalCoord();
	Camera::Update();
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

	// 回転角もビュー行列と一致させる
	rotation_.x = -pitch_;
	rotation_.y = yaw_;
	rotation_.z = 0.0f;
}
