#include "Mouse.h"

namespace MadoEngine::InputDevice
{
	Mouse::Mouse()
		: currentState_{}
		, previousState_{}
		, currentPosition_{ 0.0f, 0.0f }
		, previousPosition_{ 0.0f, 0.0f }
		, relativeDelta_{ 0.0f, 0.0f }
		, wheelDelta_(0.0f)
		, currentWheelDelta_(0.0f)
		, isRelativeMode_(false)
		, isRelativeCenterInitialized_(false)
	{
	}

	Mouse::~Mouse()
	{
		ClipCursor(nullptr);
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
		relativeDelta_ = { 0.0f, 0.0f };

		POINT cursorPos;
		if (GetCursorPos(&cursorPos))
		{
			if (ScreenToClient(hwnd, &cursorPos))
			{
				currentPosition_.x = static_cast<float>(cursorPos.x);
				currentPosition_.y = static_cast<float>(cursorPos.y);
			}
		}

		if (isRelativeMode_)
		{
			const bool hasFocus = GetForegroundWindow() == hwnd;
			POINT center = {};
			RECT clipRect = {};
			if (hasFocus && TryGetClientCenter(hwnd, center) && TryGetClientScreenRect(hwnd, clipRect))
			{
				ClipCursor(&clipRect);

				POINT screenCursorPos = {};
				if (GetCursorPos(&screenCursorPos))
				{
					if (isRelativeCenterInitialized_)
					{
						relativeDelta_.x = static_cast<float>(screenCursorPos.x - center.x);
						relativeDelta_.y = static_cast<float>(screenCursorPos.y - center.y);
					}

					SetCursorPos(center.x, center.y);
					POINT clientCenter = center;
					if (ScreenToClient(hwnd, &clientCenter))
					{
						currentPosition_.x = static_cast<float>(clientCenter.x);
						currentPosition_.y = static_cast<float>(clientCenter.y);
						previousPosition_ = currentPosition_;
					}

					isRelativeCenterInitialized_ = true;
				}
			}
			else
			{
				ClipCursor(nullptr);
				isRelativeCenterInitialized_ = false;
			}
		}
		else
		{
			ClipCursor(nullptr);
			isRelativeCenterInitialized_ = false;
		}

		currentWheelDelta_ = wheelDelta_;
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
		if (isRelativeMode_)
		{
			return relativeDelta_;
		}

		return Vector2
		{
			currentPosition_.x - previousPosition_.x,
			currentPosition_.y - previousPosition_.y
		};
	}

	float Mouse::GetWheelDelta() const
	{
		return currentWheelDelta_;
	}

	void Mouse::AddWheelDelta(float delta)
	{
		wheelDelta_ += delta;
	}

	void Mouse::SetRelativeMode(bool enable)
	{
		if (isRelativeMode_ == enable)
		{
			return;
		}

		isRelativeMode_ = enable;
		isRelativeCenterInitialized_ = false;
		relativeDelta_ = { 0.0f, 0.0f };

		if (!isRelativeMode_)
		{
			ClipCursor(nullptr);
		}
	}

	bool Mouse::IsRelativeMode() const
	{
		return isRelativeMode_;
	}

	bool Mouse::TryGetClientCenter(HWND hwnd, POINT& center) const
	{
		RECT clientRect = {};
		if (!GetClientRect(hwnd, &clientRect))
		{
			return false;
		}

		center.x = (clientRect.left + clientRect.right) / 2;
		center.y = (clientRect.top + clientRect.bottom) / 2;
		return ClientToScreen(hwnd, &center) != FALSE;
	}

	bool Mouse::TryGetClientScreenRect(HWND hwnd, RECT& rect) const
	{
		if (!GetClientRect(hwnd, &rect))
		{
			return false;
		}

		POINT leftTop = { rect.left, rect.top };
		POINT rightBottom = { rect.right, rect.bottom };
		if (!ClientToScreen(hwnd, &leftTop) || !ClientToScreen(hwnd, &rightBottom))
		{
			return false;
		}

		rect.left = leftTop.x;
		rect.top = leftTop.y;
		rect.right = rightBottom.x;
		rect.bottom = rightBottom.y;
		return true;
	}
}
