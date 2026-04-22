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

		void Update(HWND hwnd, float deltaTime = 1.0f / 60.0f);

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
		mutable std::unordered_map<std::string, bool> pressLoggedFlags_; // Pressログ出力済みフラグ

		bool CheckAnyPress(const InputAction& action) const;
		bool CheckAnyTrigger(const InputAction& action) const;
		bool CheckAnyRelease(const InputAction& action) const;

		// 文字列を小文字に変換するヘルパー関数
		std::string ToLower(const std::string& str) const;

		bool useLogger_ = true; // ロガーを使用するかどうかのフラグ
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
		// マウスマクロ: MOUSE_LEFT(0x10000), MOUSE_RIGHT(0x10001), MOUSE_MIDDLE(0x10002)
		// ゲームパッドマクロ: 0x0001-0x8000 (ビットフラグ)
		// キーボードマクロ: DIK_* (0x01-0xD3)

		for (int key : keyList)
		{
			// マウス: 0x10000-0x10002
			if (key >= 0x10000 && key <= 0x10002)
			{
				mouseKeys.push_back(key);
			}
			// ゲームパッドのビットフラグ: 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x1000, 0x2000, 0x4000, 0x8000
			// 2のべき乗かつ0x0001-0x8000の範囲ならゲームパッド
			else if (key >= 0x0001 && key <= 0x8000 && (key & (key - 1)) == 0)
			{
				gamePadKeys.push_back(key);
			}
			// キーボード: DIK_* (0x01-0xD3)
			else if (key >= 0x01 && key <= 0xD3)
			{
				keybordKeys.push_back(key);
			}
		}

		SetInput(actionName, keybordKeys, gamePadKeys, mouseKeys);
	}
}
