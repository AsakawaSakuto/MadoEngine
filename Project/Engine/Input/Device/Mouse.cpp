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

		currentState_[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;  // MOUSE_LEFT
		currentState_[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;  // MOUSE_RIGHT
		currentState_[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;  // MOUSE_MIDDLE

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
		// マクロ値(0x10000-0x10002)を内部インデックス(0-2)に変換
		int index = button - 0x10000;
		if (index < 0 || index >= BUTTON_COUNT)
		{
			return false;
		}
		return currentState_[index];
	}

	bool Mouse::IsTrigger(int button) const
	{
		// マクロ値(0x10000-0x10002)を内部インデックス(0-2)に変換
		int index = button - 0x10000;
		if (index < 0 || index >= BUTTON_COUNT)
		{
			return false;
		}
		return currentState_[index] && !previousState_[index];
	}

	bool Mouse::IsRelease(int button) const
	{
		// マクロ値(0x10000-0x10002)を内部インデックス(0-2)に変換
		int index = button - 0x10000;
		if (index < 0 || index >= BUTTON_COUNT)
		{
			return false;
		}
		return !currentState_[index] && previousState_[index];
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
