#pragma once
#include <Windows.h>
#include "Math/Vector2.h"

// 左クリック
#define MOUSE_L 0x10000
// 右クリック
#define MOUSE_R 0x10001
// 中クリック
#define MOUSE_M 0x10002

namespace MadoEngine
{
	class Mouse
	{
	public:
		Mouse();
		~Mouse();

		void Update(HWND hwnd);

		bool IsPress(int button) const;
		bool IsTrigger(int button) const;
		bool IsRelease(int button) const;

		Vector2 GetPosition() const;
		Vector2 GetDelta() const;
		float GetWheelDelta() const;

	private:
		static constexpr int BUTTON_COUNT = 3;
		bool currentState_[BUTTON_COUNT];
		bool previousState_[BUTTON_COUNT];

		Vector2 currentPosition_;
		Vector2 previousPosition_;
		float wheelDelta_;
	};
}
