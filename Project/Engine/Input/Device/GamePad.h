#pragma once
#include <Windows.h>
#include <Xinput.h>
#include "Math/Vector2.h"
#include "Utility/Easing/Easing.h"
#include "Utility/GameTimer/GameTimer.h"

#pragma comment(lib, "xinput.lib")

#define GAMEPAD_UP      0x0001
#define GAMEPAD_DOWN    0x0002
#define GAMEPAD_LEFT    0x0004
#define GAMEPAD_RIGHT   0x0008
#define GAMEPAD_START   0x0010
#define GAMEPAD_BACK    0x0020
#define GAMEPAD_L       0x0040
#define GAMEPAD_R       0x0080
#define GAMEPAD_STICK_L 0x0100
#define GAMEPAD_STICK_R 0x0200
#define GAMEPAD_A       0x1000
#define GAMEPAD_B       0x2000
#define GAMEPAD_X       0x4000
#define GAMEPAD_Y       0x8000

struct VibrationDesc {
	float LeftMotor;   // 振動の強さ（0.0f～1.0f）
	float RightMotor;  // 振動の強さ（0.0f～1.0f）
	float time;        // 振動の持続時間（秒）
	EaseType easeType; // 振動の強さを時間で補間するイージングタイプ
};

namespace MadoEngine
{
	class GamePad
	{
	public:
		GamePad(int playerIndex = 0);
		~GamePad();

		void Update(float deltaTime = 1.0f / 60.0f);

		bool IsPress(int button) const;
		bool IsTrigger(int button) const;
		bool IsRelease(int button) const;

		Vector2 GetLeftStick() const;
		Vector2 GetRightStick() const;
		float GetLeftTrigger() const;
		float GetRightTrigger() const;

		bool IsConnected() const;

		/// @brief 振動を設定する
		/// @param leftMotor 左モーターの振動の強さ（0.0f～1.0f）
		/// @param rightMotor 右モーターの振動の強さ（0.0f～1.0f）
		/// @param time 振動の持続時間（秒）
		/// @param easeType 振動の強さを時間で補間するイージングタイプ
		void SetVibration(float leftMotor, float rightMotor, float time, EaseType easeType = EaseType::Linear);
		void SetVibration(VibrationDesc desc);

	private:
		int playerIndex_;
		XINPUT_STATE currentState_;
		XINPUT_STATE previousState_;
		bool connected_;

		// 振動管理用
		VibrationDesc vibrationDesc_;
		GameTimer vibrationTimer_;

		static constexpr float STICK_DEADZONE = 0.15f;
		static constexpr float TRIGGER_THRESHOLD = 0.1f;

		float NormalizeStickValue(SHORT value, SHORT deadzone) const;
		float NormalizeTriggerValue(BYTE value) const;
		void UpdateVibration(float deltaTime);
	};
}