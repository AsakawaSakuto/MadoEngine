#pragma once
#include <Windows.h>
#include <Xinput.h>
#include "Math/Vector2.h"

#pragma comment(lib, "xinput.lib")

// GamePad Button Definitions
#define GAMEPAD_DPAD_UP         0x0001
#define GAMEPAD_DPAD_DOWN       0x0002
#define GAMEPAD_DPAD_LEFT       0x0004
#define GAMEPAD_DPAD_RIGHT      0x0008
#define GAMEPAD_START           0x0010
#define GAMEPAD_BACK            0x0020
#define GAMEPAD_LEFT_THUMB      0x0040
#define GAMEPAD_RIGHT_THUMB     0x0080
#define GAMEPAD_LEFT_SHOULDER   0x0100
#define GAMEPAD_RIGHT_SHOULDER  0x0200
#define GAMEPAD_A               0x1000
#define GAMEPAD_B               0x2000
#define GAMEPAD_X               0x4000
#define GAMEPAD_Y               0x8000

namespace MadoEngine
{
	class GamePad
	{
	public:
		GamePad(int playerIndex = 0);
		~GamePad();

		void Update();

		bool IsPress(int button) const;
		bool IsTrigger(int button) const;
		bool IsRelease(int button) const;

		Vector2 GetLeftStick() const;
		Vector2 GetRightStick() const;
		float GetLeftTrigger() const;
		float GetRightTrigger() const;

		bool IsConnected() const;

		void SetVibration(float leftMotor, float rightMotor);

	private:
		int playerIndex_;
		XINPUT_STATE currentState_;
		XINPUT_STATE previousState_;
		bool connected_;

		static constexpr float STICK_DEADZONE = 0.15f;
		static constexpr float TRIGGER_THRESHOLD = 0.1f;

		float NormalizeStickValue(SHORT value, SHORT deadzone) const;
		float NormalizeTriggerValue(BYTE value) const;
	};
}
