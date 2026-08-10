#include "PlayerController.h"
#include "Input/MyInput.h"
#include <cmath>

namespace Player {

	void Controller::Update() {
		moveInput_ = {};

		// GamePadとKeyboardの入力を同じ移動ベクトルへ合成
		Vector2 input = MyInput::GetGamePad()->GetLeftStick();
		input.x += MyInput::Press("Right") ? 1.0f : 0.0f;
		input.x -= MyInput::Press("Left") ? 1.0f : 0.0f;
		input.y += MyInput::Press("Up") ? 1.0f : 0.0f;
		input.y -= MyInput::Press("Down") ? 1.0f : 0.0f;

		const float inputLengthSq = input.x * input.x + input.y * input.y;
		if (inputLengthSq > 1.0f) {
			// 斜め入力で移動量が増えないよう入力を単位円内へ正規化
			const float inputLength = std::sqrt(inputLengthSq);
			input.x /= inputLength;
			input.y /= inputLength;
		}

		moveInput_.move = input;
		moveInput_.isJumpTriggered = MyInput::Trigger("Jump");
		moveInput_.isCrouching = MyInput::Press("Crouching");
		// スライド開始判定に必要なしゃがみ入力の立ち上がりを前フレーム状態から生成
		moveInput_.isCrouchingStarted = moveInput_.isCrouching && !wasCrouching_;

		wasCrouching_ = moveInput_.isCrouching;
	}
}
