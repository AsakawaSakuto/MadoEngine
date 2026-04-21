#include "Keybord.h"

namespace MadoEngine
{
	Keybord::Keybord()
		: currentState_{}
		, previousState_{}
	{
	}

	Keybord::~Keybord()
	{
	}

	int Keybord::ConvertDIKToVK(int key) const
	{
		// DIK_XXXキーコードをVK_XXXに変換
		switch (key)
		{
		case DIK_ESCAPE: return VK_ESCAPE;
		case DIK_1: return '1';
		case DIK_2: return '2';
		case DIK_3: return '3';
		case DIK_4: return '4';
		case DIK_5: return '5';
		case DIK_6: return '6';
		case DIK_7: return '7';
		case DIK_8: return '8';
		case DIK_9: return '9';
		case DIK_0: return '0';
		case DIK_MINUS: return VK_OEM_MINUS;
		case DIK_EQUALS: return VK_OEM_PLUS;
		case DIK_BACK: return VK_BACK;
		case DIK_TAB: return VK_TAB;
		case DIK_Q: return 'Q';
		case DIK_W: return 'W';
		case DIK_E: return 'E';
		case DIK_R: return 'R';
		case DIK_T: return 'T';
		case DIK_Y: return 'Y';
		case DIK_U: return 'U';
		case DIK_I: return 'I';
		case DIK_O: return 'O';
		case DIK_P: return 'P';
		case DIK_LBRACKET: return VK_OEM_4;
		case DIK_RBRACKET: return VK_OEM_6;
		case DIK_RETURN: return VK_RETURN;
		case DIK_LCONTROL: return VK_LCONTROL;
		case DIK_A: return 'A';
		case DIK_S: return 'S';
		case DIK_D: return 'D';
		case DIK_F: return 'F';
		case DIK_G: return 'G';
		case DIK_H: return 'H';
		case DIK_J: return 'J';
		case DIK_K: return 'K';
		case DIK_L: return 'L';
		case DIK_SEMICOLON: return VK_OEM_1;
		case DIK_APOSTROPHE: return VK_OEM_7;
		case DIK_GRAVE: return VK_OEM_3;
		case DIK_LSHIFT: return VK_LSHIFT;
		case DIK_BACKSLASH: return VK_OEM_5;
		case DIK_Z: return 'Z';
		case DIK_X: return 'X';
		case DIK_C: return 'C';
		case DIK_V: return 'V';
		case DIK_B: return 'B';
		case DIK_N: return 'N';
		case DIK_M: return 'M';
		case DIK_COMMA: return VK_OEM_COMMA;
		case DIK_PERIOD: return VK_OEM_PERIOD;
		case DIK_SLASH: return VK_OEM_2;
		case DIK_RSHIFT: return VK_RSHIFT;
		case DIK_MULTIPLY: return VK_MULTIPLY;
		case DIK_LMENU: return VK_LMENU;
		case DIK_SPACE: return VK_SPACE;
		case DIK_CAPITAL: return VK_CAPITAL;
		case DIK_F1: return VK_F1;
		case DIK_F2: return VK_F2;
		case DIK_F3: return VK_F3;
		case DIK_F4: return VK_F4;
		case DIK_F5: return VK_F5;
		case DIK_F6: return VK_F6;
		case DIK_F7: return VK_F7;
		case DIK_F8: return VK_F8;
		case DIK_F9: return VK_F9;
		case DIK_F10: return VK_F10;
		case DIK_NUMLOCK: return VK_NUMLOCK;
		case DIK_SCROLL: return VK_SCROLL;
		case DIK_NUMPAD7: return VK_NUMPAD7;
		case DIK_NUMPAD8: return VK_NUMPAD8;
		case DIK_NUMPAD9: return VK_NUMPAD9;
		case DIK_SUBTRACT: return VK_SUBTRACT;
		case DIK_NUMPAD4: return VK_NUMPAD4;
		case DIK_NUMPAD5: return VK_NUMPAD5;
		case DIK_NUMPAD6: return VK_NUMPAD6;
		case DIK_ADD: return VK_ADD;
		case DIK_NUMPAD1: return VK_NUMPAD1;
		case DIK_NUMPAD2: return VK_NUMPAD2;
		case DIK_NUMPAD3: return VK_NUMPAD3;
		case DIK_NUMPAD0: return VK_NUMPAD0;
		case DIK_DECIMAL: return VK_DECIMAL;
		case DIK_F11: return VK_F11;
		case DIK_F12: return VK_F12;
		case DIK_NUMPADENTER: return VK_RETURN;
		case DIK_RCONTROL: return VK_RCONTROL;
		case DIK_DIVIDE: return VK_DIVIDE;
		case DIK_RMENU: return VK_RMENU;
		case DIK_HOME: return VK_HOME;
		case DIK_UP: return VK_UP;
		case DIK_PRIOR: return VK_PRIOR;
		case DIK_LEFT: return VK_LEFT;
		case DIK_RIGHT: return VK_RIGHT;
		case DIK_END: return VK_END;
		case DIK_DOWN: return VK_DOWN;
		case DIK_NEXT: return VK_NEXT;
		case DIK_INSERT: return VK_INSERT;
		case DIK_DELETE: return VK_DELETE;
		default: return key; // VK_XXXまたは文字コードとしてそのまま使用
		}
	}

	void Keybord::Update()
	{
		previousState_ = currentState_;

		for (int i = 0; i < KEY_COUNT; ++i)
		{
			currentState_[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
		}
	}

	bool Keybord::IsPress(int key) const
	{
		int vkKey = ConvertDIKToVK(key);
		if (vkKey < 0 || vkKey >= KEY_COUNT)
		{
			return false;
		}
		return currentState_[vkKey];
	}

	bool Keybord::IsTrigger(int key) const
	{
		int vkKey = ConvertDIKToVK(key);
		if (vkKey < 0 || vkKey >= KEY_COUNT)
		{
			return false;
		}
		return currentState_[vkKey] && !previousState_[vkKey];
	}

	bool Keybord::IsRelease(int key) const
	{
		int vkKey = ConvertDIKToVK(key);
		if (vkKey < 0 || vkKey >= KEY_COUNT)
		{
			return false;
		}
		return !currentState_[vkKey] && previousState_[vkKey];
	}
}
