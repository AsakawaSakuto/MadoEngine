#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include "Device/Keybord.h"
#include "Device/Mouse.h"
#include "Device/GamePad.h"

namespace MadoEngine
{
	class InputManager
	{
	public:
		static InputManager* GetInstance();

		void Initialize();
		void Finalize();

		~InputManager() = default;

		void Update(HWND hwnd);

		// 入力アクションに複数のキー/ボタンを登録
		// 例: SetInput("Jump", {DIK_SPACE, DIK_Z}, {GAMEPAD_A}, {MOUSE_LEFT});
		void SetInput(
			const std::string& actionName,
			const std::vector<int>& keybordKeys = {},
			const std::vector<int>& gamePadButtons = {},
			const std::vector<int>& mouseButtons = {}
		);

		// 便利なオーバーロード - 可変長引数で直接指定
		template<typename... Args>
		void SetInputKeys(const std::string& actionName, Args... keys);

		// 登録されたアクションの入力判定
		bool IsPress(const std::string& actionName) const;
		bool IsTrigger(const std::string& actionName) const;
		bool IsRelease(const std::string& actionName) const;

		// エイリアス（要求仕様に合わせた名前）
		inline bool Trigger(const std::string& actionName) const { return IsTrigger(actionName); }
		inline bool Press(const std::string& actionName) const { return IsPress(actionName); }
		inline bool Release(const std::string& actionName) const { return IsRelease(actionName); }

		// デバイスへの直接アクセス
		Keybord* GetKeybord() { return keybord_.get(); }
		Mouse* GetMouse() { return mouse_.get(); }
		GamePad* GetGamePad() { return gamePad_.get(); }

	private:
		InputManager() = default;
		InputManager(const InputManager&) = delete;
		InputManager& operator=(const InputManager&) = delete;
		struct InputAction
		{
			std::vector<int> keybordKeys;
			std::vector<int> gamePadButtons;
			std::vector<int> mouseButtons;
		};

		std::unique_ptr<Keybord> keybord_;
		std::unique_ptr<Mouse> mouse_;
		std::unique_ptr<GamePad> gamePad_;

		std::unordered_map<std::string, InputAction> inputActions_;

		bool CheckAnyPress(const InputAction& action) const;
		bool CheckAnyTrigger(const InputAction& action) const;
		bool CheckAnyRelease(const InputAction& action) const;

		// 文字列を小文字に変換するヘルパー関数
		std::string ToLower(const std::string& str) const;
	};

	// テンプレート実装
	template<typename... Args>
	void InputManager::SetInputKeys(const std::string& actionName, Args... keys)
	{
		std::vector<int> keyList = { keys... };
		std::vector<int> keybordKeys;
		std::vector<int> gamePadKeys;
		std::vector<int> mouseKeys;

		// キーコードを範囲で判定して分類
		for (int key : keyList)
		{
			if (key >= 0x01 && key <= 0xD3) // DIK_XXX range
			{
				keybordKeys.push_back(key);
			}
			else if (key >= 0x0001 && key <= 0x8000) // GAMEPAD_XXX range
			{
				gamePadKeys.push_back(key);
			}
			else if (key >= 0 && key <= 2) // MOUSE_XXX range (0-2)
			{
				mouseKeys.push_back(key);
			}
		}

		SetInput(actionName, keybordKeys, gamePadKeys, mouseKeys);
	}
}
