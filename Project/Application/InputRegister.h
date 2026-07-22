#pragma once
#include "../Engine/UtilityHeaders.h"

/// @brief ゲーム内の入力を登録する関数
void RegisterInput() {

	MyInput::RegisterInput("Up", { DIK_UP,DIK_W }, { GAMEPAD_UP });
	MyInput::RegisterInput("Down", { DIK_DOWN,DIK_S }, { GAMEPAD_DOWN });
	MyInput::RegisterInput("Left", { DIK_LEFT,DIK_A }, { GAMEPAD_LEFT });
	MyInput::RegisterInput("Right", { DIK_RIGHT,DIK_D }, { GAMEPAD_RIGHT });

	MyInput::RegisterInput("Jump", { DIK_SPACE,DIK_Z }, { GAMEPAD_A, GAMEPAD_STICK_L });
	MyInput::RegisterInput("Crouching", { DIK_LSHIFT }, { GAMEPAD_R });

	MyInput::RegisterInput("Interact", { DIK_E }, { GAMEPAD_X });
	MyInput::RegisterInput("Decision", { DIK_SPACE }, { GAMEPAD_A });

	MyInput::RegisterInput("Pause", { DIK_ESCAPE }, { GAMEPAD_START });

}