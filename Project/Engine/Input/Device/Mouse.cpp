#include "Mouse.h"

namespace MadoEngine
{
	Mouse::Mouse()
		: currentState_{}
		, previousState_{}
		, currentPosition_{ 0.0f, 0.0f }
		, previousPosition_{ 0.0f, 0.0f }
		, wheelDelta_(0.0f)
	{
	}

	Mouse::~Mouse()
	{
	}

	void Mouse::Update(HWND hwnd)
	{
		for (int i = 0; i < BUTTON_COUNT; ++i)
		{
			previousState_[i] = currentState_[i];
		}

		currentState_[MOUSE_LEFT] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		currentState_[MOUSE_RIGHT] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
		currentState_[MOUSE_MIDDLE] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

		previousPosition_ = currentPosition_;

		POINT cursorPos;
		if (GetCursorPos(&cursorPos))
		{
			if (ScreenToClient(hwnd, &cursorPos))
			{
				currentPosition_.x = static_cast<float>(cursorPos.x);
				currentPosition_.y = static_cast<float>(cursorPos.y);
			}
		}

		wheelDelta_ = 0.0f;
	}

	bool Mouse::IsPress(int button) const
	{
		if (button < 0 || button >= BUTTON_COUNT)
		{
			return false;
		}
		return currentState_[button];
	}

	bool Mouse::IsTrigger(int button) const
	{
		if (button < 0 || button >= BUTTON_COUNT)
		{
			return false;
		}
		return currentState_[button] && !previousState_[button];
	}

	bool Mouse::IsRelease(int button) const
	{
		if (button < 0 || button >= BUTTON_COUNT)
		{
			return false;
		}
		return !currentState_[button] && previousState_[button];
	}

	Vector2 Mouse::GetPosition() const
	{
		return currentPosition_;
	}

	Vector2 Mouse::GetDelta() const
	{
		return Vector2
		{
			currentPosition_.x - previousPosition_.x,
			currentPosition_.y - previousPosition_.y
		};
	}

	float Mouse::GetWheelDelta() const
	{
		return wheelDelta_;
	}
}
