#pragma once
#include "InputManager.h"

/// @brief 入力管理の簡易ラッパー名前空間
/// InputManagerへの簡潔なアクセスを提供
namespace MyInput {

	/// @brief 中立復帰で再入力可能なスティックの横方向入力を登録
	/// @param actionName 入力の登録名
	/// @param stick 参照するスティック
	inline void RegisterStickHorizontalInput(const std::string& actionName, MadoEngine::GamePadStick stick = MadoEngine::GamePadStick::Left) {
		MadoEngine::InputManager::GetInstance().SetStickHorizontalInput(actionName, stick);
	}

	/// @brief 登録済みのスティック横方向入力を解除
	/// @param actionName 入力の登録名
	inline void RemoveStickHorizontalInput(const std::string& actionName) {
		MadoEngine::InputManager::GetInstance().RemoveStickHorizontalInput(actionName);
	}

	/// @brief 現在Frameのスティック横方向の単発入力を取得
	/// @param actionName 入力の登録名
	/// @return 左の場合は-1、右の場合は1、入力なしの場合は0
	inline int GetStickHorizontalTrigger(const std::string& actionName) {
		return MadoEngine::InputManager::GetInstance().GetStickHorizontalTrigger(actionName);
	}

	/// @brief 登録済み入力の検出結果を破棄して中立復帰を待機
	/// @param actionName 入力の登録名
	inline void ResetStickHorizontalInput(const std::string& actionName) {
		MadoEngine::InputManager::GetInstance().ResetStickHorizontalInput(actionName);
	}

	/// @brief 入力アクションに複数のキー/ボタンを登録
	/// @param actionName アクション名（例: "Jump"）
	/// @param keyboardKeys キーボードのキーコード配列（例: {DIK_SPACE, DIK_Z}）
	/// @param gamePadButtons ゲームパッドのボタン配列（例: {GAMEPAD_A}）
	/// @param mouseButtons マウスのボタン配列（例: {MOUSE_LEFT}）
	inline void RegisterInput(
		const std::string& actionName,
		const std::vector<int>& keyboardKeys = {},
		const std::vector<int>& gamePadButtons = {},
		const std::vector<int>& mouseButtons = {}) 
	{
		MadoEngine::InputManager::GetInstance().SetInput(actionName, keyboardKeys, gamePadButtons, mouseButtons);
	}

	/// @brief 入力アクションに複数のキー/ボタンを登録（可変長引数版）
	/// @param actionName アクション名（例: "Jump"）
	/// @param keys キーコード（例: DIK_SPACE, DIK_Z, GAMEPAD_A, MOUSE_LEFT）
	template<typename... Args>
	void RegisterInputKeys(const std::string& actionName, Args... keys) {
		MadoEngine::InputManager::GetInstance().SetInputKeys(actionName, keys...);
	}

	/// @brief 登録されたアクションが押された瞬間か判定
	/// @param actionName アクション名
	/// @return 押された瞬間ならtrue
	inline bool Trigger(const std::string& actionName) {
		return MadoEngine::InputManager::GetInstance().Trigger(actionName);
	}

	/// @brief 登録されたアクションが押されているか判定
	/// @param actionName アクション名
	/// @return 押されている間true
	inline bool Press(const std::string& actionName) {
		return MadoEngine::InputManager::GetInstance().Press(actionName);
	}

	/// @brief 登録されたアクションが離された瞬間か判定
	/// @param actionName アクション名
	/// @return 離された瞬間ならtrue
	inline bool Release(const std::string& actionName) {
		return MadoEngine::InputManager::GetInstance().Release(actionName);
	}

	/// @brief キーボードへ直接アクセス
	/// @return Keyboardインスタンスへのポインタ
	inline MadoEngine::InputDevice::Keyboard* GetKeyboard() {
		return MadoEngine::InputManager::GetInstance().GetKeyboard();
	}

	/// @brief マウスへ直接アクセス
	/// @return Mouseインスタンスへのポインタ
	inline MadoEngine::InputDevice::Mouse* GetMouse() {
		return MadoEngine::InputManager::GetInstance().GetMouse();
	}

	/// @brief ゲームパッドへ直接アクセス
	/// @return GamePadインスタンスへのポインタ
	inline MadoEngine::InputDevice::GamePad* GetGamePad() {
		return MadoEngine::InputManager::GetInstance().GetGamePad();
	}
}

