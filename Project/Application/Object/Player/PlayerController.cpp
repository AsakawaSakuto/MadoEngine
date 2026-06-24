#include "PlayerController.h"
#include "Input/MyInput.h"
#include <cmath>

void PlayerController::Update() {
	moveInput_ = {};

	Vector2 input = MyInput::GetGamePad()->GetLeftStick();
	input.x += MyInput::Press("Right") ? 1.0f : 0.0f;
	input.x -= MyInput::Press("Left") ? 1.0f : 0.0f;
	input.y += MyInput::Press("Up") ? 1.0f : 0.0f;
	input.y -= MyInput::Press("Down") ? 1.0f : 0.0f;

	const float inputLengthSq = input.x * input.x + input.y * input.y;
	if (inputLengthSq > 1.0f) {
		const float inputLength = std::sqrt(inputLengthSq);
		input.x /= inputLength;
		input.y /= inputLength;
	}

	moveInput_.move = input;
	moveInput_.isJumpTriggered = MyInput::Trigger("Jump");
	moveInput_.isCrouching = MyInput::Press("Crouching");
	moveInput_.isCrouchingStarted = moveInput_.isCrouching && !wasCrouching_;

	wasCrouching_ = moveInput_.isCrouching;
}
