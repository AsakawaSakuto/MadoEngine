#include "GamePad.h"
#include <cmath>

namespace MadoEngine
{
	GamePad::GamePad(int playerIndex)
		: playerIndex_(playerIndex)
		, currentState_{}
		, previousState_{}
		, connected_(false)
	{
		if (playerIndex_ < 0 || playerIndex_ >= XUSER_MAX_COUNT)
		{
			playerIndex_ = 0;
		}
	}

	GamePad::~GamePad()
	{
		SetVibration(0.0f, 0.0f);
	}

	void GamePad::Update()
	{
		previousState_ = currentState_;

		DWORD result = XInputGetState(playerIndex_, &currentState_);
		connected_ = (result == ERROR_SUCCESS);

		if (!connected_)
		{
			ZeroMemory(&currentState_, sizeof(XINPUT_STATE));
		}
	}

	bool GamePad::IsPress(int button) const
	{
		if (!connected_)
		{
			return false;
		}
		return (currentState_.Gamepad.wButtons & button) != 0;
	}

	bool GamePad::IsTrigger(int button) const
	{
		if (!connected_)
		{
			return false;
		}
		bool currentPress = (currentState_.Gamepad.wButtons & button) != 0;
		bool previousPress = (previousState_.Gamepad.wButtons & button) != 0;
		return currentPress && !previousPress;
	}

	bool GamePad::IsRelease(int button) const
	{
		if (!connected_)
		{
			return false;
		}
		bool currentPress = (currentState_.Gamepad.wButtons & button) != 0;
		bool previousPress = (previousState_.Gamepad.wButtons & button) != 0;
		return !currentPress && previousPress;
	}

	Vector2 GamePad::GetLeftStick() const
	{
		if (!connected_)
		{
			return Vector2{ 0.0f, 0.0f };
		}

		float x = NormalizeStickValue(currentState_.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
		float y = NormalizeStickValue(currentState_.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

		return Vector2{ x, y };
	}

	Vector2 GamePad::GetRightStick() const
	{
		if (!connected_)
		{
			return Vector2{ 0.0f, 0.0f };
		}

		float x = NormalizeStickValue(currentState_.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
		float y = NormalizeStickValue(currentState_.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

		return Vector2{ x, y };
	}

	float GamePad::GetLeftTrigger() const
	{
		if (!connected_)
		{
			return 0.0f;
		}
		return NormalizeTriggerValue(currentState_.Gamepad.bLeftTrigger);
	}

	float GamePad::GetRightTrigger() const
	{
		if (!connected_)
		{
			return 0.0f;
		}
		return NormalizeTriggerValue(currentState_.Gamepad.bRightTrigger);
	}

	bool GamePad::IsConnected() const
	{
		return connected_;
	}

	void GamePad::SetVibration(float leftMotor, float rightMotor)
	{
		if (!connected_)
		{
			return;
		}

		leftMotor = (leftMotor < 0.0f) ? 0.0f : (leftMotor > 1.0f) ? 1.0f : leftMotor;
		rightMotor = (rightMotor < 0.0f) ? 0.0f : (rightMotor > 1.0f) ? 1.0f : rightMotor;

		XINPUT_VIBRATION vibration;
		vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
		vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);

		XInputSetState(playerIndex_, &vibration);
	}

	float GamePad::NormalizeStickValue(SHORT value, SHORT deadzone) const
	{
		if (value > deadzone)
		{
			return static_cast<float>(value - deadzone) / (32767.0f - deadzone);
		}
		else if (value < -deadzone)
		{
			return static_cast<float>(value + deadzone) / (32767.0f - deadzone);
		}
		return 0.0f;
	}

	float GamePad::NormalizeTriggerValue(BYTE value) const
	{
		if (value < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		{
			return 0.0f;
		}
		return static_cast<float>(value - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / (255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
	}
}
