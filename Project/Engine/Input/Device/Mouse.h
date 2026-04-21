#pragma once
#include <Windows.h>
#include "Math/Vector2.h"

// Mouse Button Definitions
#define MOUSE_LEFT      0
#define MOUSE_RIGHT     1
#define MOUSE_MIDDLE    2

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
