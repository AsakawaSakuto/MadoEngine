#pragma once
#include "InputManager.h"

/// @brief 入力管理の簡易ラッパー名前空間
/// InputManagerへの簡潔なアクセスを提供
namespace Input {

	/// @brief 入力アクションに複数のキー/ボタンを登録
	/// @param actionName アクション名（例: "Jump"）
	/// @param keybordKeys キーボードのキーコード配列（例: {DIK_SPACE, DIK_Z}）
	/// @param gamePadButtons ゲームパッドのボタン配列（例: {GAMEPAD_A}）
	/// @param mouseButtons マウスのボタン配列（例: {MOUSE_LEFT}）
	inline void SetInput(
		const std::string& actionName,
		const std::vector<int>& keybordKeys = {},
		const std::vector<int>& gamePadButtons = {},
		const std::vector<int>& mouseButtons = {}) 
	{
		MadoEngine::InputManager::GetInstance()->SetInput(actionName, keybordKeys, gamePadButtons, mouseButtons);
	}

	/// @brief 入力アクションに複数のキー/ボタンを登録（可変長引数版）
	/// @param actionName アクション名（例: "Jump"）
	/// @param keys キーコード（例: DIK_SPACE, DIK_Z, GAMEPAD_A, MOUSE_LEFT）
	template<typename... Args>
	void SetInputKeys(const std::string& actionName, Args... keys) {
		MadoEngine::InputManager::GetInstance()->SetInputKeys(actionName, keys...);
	}

	/// @brief 登録されたアクションが押された瞬間か判定
	/// @param actionName アクション名
	/// @return 押された瞬間ならtrue
	inline bool Trigger(const std::string& actionName) {
		return MadoEngine::InputManager::GetInstance()->Trigger(actionName);
	}

	/// @brief 登録されたアクションが押されているか判定
	/// @param actionName アクション名
	/// @return 押されている間true
	inline bool Press(const std::string& actionName) {
		return MadoEngine::InputManager::GetInstance()->Press(actionName);
	}

	/// @brief 登録されたアクションが離された瞬間か判定
	/// @param actionName アクション名
	/// @return 離された瞬間ならtrue
	inline bool Release(const std::string& actionName) {
		return MadoEngine::InputManager::GetInstance()->Release(actionName);
	}

	/// @brief キーボードへ直接アクセス
	/// @return Keybordインスタンスへのポインタ
	inline MadoEngine::Keybord* GetKeybord() {
		return MadoEngine::InputManager::GetInstance()->GetKeybord();
	}

	/// @brief マウスへ直接アクセス
	/// @return Mouseインスタンスへのポインタ
	inline MadoEngine::Mouse* GetMouse() {
		return MadoEngine::InputManager::GetInstance()->GetMouse();
	}

	/// @brief ゲームパッドへ直接アクセス
	/// @return GamePadインスタンスへのポインタ
	inline MadoEngine::GamePad* GetGamePad() {
		return MadoEngine::InputManager::GetInstance()->GetGamePad();
	}
}

