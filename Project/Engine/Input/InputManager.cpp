#include "InputManager.h"

namespace MadoEngine
{
	InputManager* InputManager::GetInstance()
	{
		static InputManager instance;
		return &instance;
	}

	void InputManager::Initialize()
	{
		keybord_ = std::make_unique<Keybord>();
		mouse_ = std::make_unique<Mouse>();
		gamePad_ = std::make_unique<GamePad>();
	}

	void InputManager::Finalize()
	{
		keybord_.reset();
		mouse_.reset();
		gamePad_.reset();
		inputActions_.clear();
	}

	void InputManager::Update(HWND hwnd)
	{
		keybord_->Update();
		mouse_->Update(hwnd);
		gamePad_->Update();
	}

	void InputManager::SetInput(
		const std::string& actionName,
		const std::vector<int>& keybordKeys,
		const std::vector<int>& gamePadButtons,
		const std::vector<int>& mouseButtons)
	{
		InputAction action;
		action.keybordKeys = keybordKeys;
		action.gamePadButtons = gamePadButtons;
		action.mouseButtons = mouseButtons;

		// アクション名を小文字に変換して保存
		inputActions_[ToLower(actionName)] = action;
	}

	bool InputManager::IsPress(const std::string& actionName) const
	{
		// 小文字に変換して検索
		auto it = inputActions_.find(ToLower(actionName));
		if (it == inputActions_.end())
		{
			return false;
		}

		return CheckAnyPress(it->second);
	}

	bool InputManager::IsTrigger(const std::string& actionName) const
	{
		// 小文字に変換して検索
		auto it = inputActions_.find(ToLower(actionName));
		if (it == inputActions_.end())
		{
			return false;
		}

		return CheckAnyTrigger(it->second);
	}

	bool InputManager::IsRelease(const std::string& actionName) const
	{
		// 小文字に変換して検索
		auto it = inputActions_.find(ToLower(actionName));
		if (it == inputActions_.end())
		{
			return false;
		}

		return CheckAnyRelease(it->second);
	}

	bool InputManager::CheckAnyPress(const InputAction& action) const
	{
		// キーボードチェック
		for (int key : action.keybordKeys)
		{
			if (keybord_->IsPress(key))
			{
				return true;
			}
		}

		// ゲームパッドチェック
		for (int button : action.gamePadButtons)
		{
			if (gamePad_->IsPress(button))
			{
				return true;
			}
		}

		// マウスチェック
		for (int button : action.mouseButtons)
		{
			if (mouse_->IsPress(button))
			{
				return true;
			}
		}

		return false;
	}

	bool InputManager::CheckAnyTrigger(const InputAction& action) const
	{
		// キーボードチェック
		for (int key : action.keybordKeys)
		{
			if (keybord_->IsTrigger(key))
			{
				return true;
			}
		}

		// ゲームパッドチェック
		for (int button : action.gamePadButtons)
		{
			if (gamePad_->IsTrigger(button))
			{
				return true;
			}
		}

		// マウスチェック
		for (int button : action.mouseButtons)
		{
			if (mouse_->IsTrigger(button))
			{
				return true;
			}
		}

		return false;
	}

	bool InputManager::CheckAnyRelease(const InputAction& action) const
	{
		// キーボードチェック
		for (int key : action.keybordKeys)
		{
			if (keybord_->IsRelease(key))
			{
				return true;
			}
		}

		// ゲームパッドチェック
		for (int button : action.gamePadButtons)
		{
			if (gamePad_->IsRelease(button))
			{
				return true;
			}
		}

		// マウスチェック
		for (int button : action.mouseButtons)
		{
			if (mouse_->IsRelease(button))
			{
				return true;
			}
		}

		return false;
	}

	std::string InputManager::ToLower(const std::string& str) const
	{
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return result;
	}
}