#include "InputManager.h"
#include "Utility/Logger/Logger.h"
#include <cmath>

namespace {
	constexpr float kStickInputThreshold = 0.5f;
	constexpr float kStickNeutralThreshold = 0.2f;

	/// @brief 指定したスティックの傾きを取得
	/// @param gamePad 入力元のGamePad、未初期化の場合はnullptr
	/// @param stick 参照するスティック
	/// @return 正規化済みの傾き、未初期化の場合は中立値
	Vector2 ReadStick(const MadoEngine::InputDevice::GamePad* gamePad, MadoEngine::GamePadStick stick) {
		if (!gamePad) {
			return {};
		}

		// 生の入力値とDeadzone補正はDevice側へ集約し、管理側では正規化済みの値を使用
		if (stick == MadoEngine::GamePadStick::Right) {
			return gamePad->GetRightStick();
		}
		return gamePad->GetLeftStick();
	}

	/// @brief スティックが再入力を許可する中立範囲内か判定
	/// @param stick 正規化済みのスティックの傾き
	/// @return 中立範囲内の場合はtrue
	bool IsStickNeutral(const Vector2& stick) {
		return std::abs(stick.x) <= kStickNeutralThreshold && std::abs(stick.y) <= kStickNeutralThreshold;
	}
}

namespace MadoEngine
{
	InputManager& InputManager::GetInstance()
	{
		static InputManager instance;
		return instance;
	}

	void InputManager::Initialize() {
		keyboard_ = std::make_unique<MadoEngine::InputDevice::Keyboard>();
		mouse_ = std::make_unique<MadoEngine::InputDevice::Mouse>();
		gamePad_ = std::make_unique<MadoEngine::InputDevice::GamePad>();
		stickHorizontalInputs_.clear();
	}

	void InputManager::Finalize() {
		keyboard_.reset();
		mouse_.reset();
		gamePad_.reset();
		inputActions_.clear();
		stickHorizontalInputs_.clear();
	}

	void InputManager::Update(HWND hwnd, float deltaTime) {
		keyboard_->Update();
		mouse_->Update(hwnd);
		gamePad_->Update(deltaTime);

		// 全Deviceの状態確定後に一度だけ判定し、同じFrameの参照先すべてへ同じ結果を提供
		UpdateStickHorizontalInputs();
	}

	void InputManager::SetStickHorizontalInput(const std::string& actionName, GamePadStick stick) {
		StickHorizontalInput input;
		input.stick = stick;

		// 登録前から倒されているスティックを新規の移動入力として扱わない初期状態
		input.isReady = IsStickNeutral(ReadStick(gamePad_.get(), stick));
		stickHorizontalInputs_[ToLower(actionName)] = input;
	}

	void InputManager::RemoveStickHorizontalInput(const std::string& actionName) {
		stickHorizontalInputs_.erase(ToLower(actionName));
	}

	int InputManager::GetStickHorizontalTrigger(const std::string& actionName) const {
		const auto it = stickHorizontalInputs_.find(ToLower(actionName));
		if (it == stickHorizontalInputs_.end()) {
			return 0;
		}
		return it->second.triggeredDirection;
	}

	void InputManager::ResetStickHorizontalInput(const std::string& actionName) {
		const auto it = stickHorizontalInputs_.find(ToLower(actionName));
		if (it == stickHorizontalInputs_.end()) {
			return;
		}

		// 表示切り替えで古い入力を破棄し、他の登録名の再入力状態には影響しないリセット
		StickHorizontalInput& input = it->second;
		input.triggeredDirection = 0;
		input.isReady = IsStickNeutral(ReadStick(gamePad_.get(), input.stick));
	}

	void InputManager::UpdateStickHorizontalInputs() {
		const Vector2 leftStick = ReadStick(gamePad_.get(), GamePadStick::Left);
		const Vector2 rightStick = ReadStick(gamePad_.get(), GamePadStick::Right);
		for (auto& [name, input] : stickHorizontalInputs_) {
			(void)name;
			input.triggeredDirection = 0;
			Vector2 stick = leftStick;
			if (input.stick == GamePadStick::Right) {
				stick = rightStick;
			}

			// 狭い中立範囲への復帰でのみ再入力を許可し、閾値付近の揺れによる連続検出を防止
			if (IsStickNeutral(stick)) {
				input.isReady = true;
				continue;
			}
			if (!input.isReady) {
				continue;
			}

			// 反対方向への直接の切り返しでも再発火しないよう、左右共通の中立待ちへ遷移
			if (stick.x <= -kStickInputThreshold) {
				input.triggeredDirection = -1;
				input.isReady = false;
			} else if (stick.x >= kStickInputThreshold) {
				input.triggeredDirection = 1;
				input.isReady = false;
			}
		}
	}

	void InputManager::SetInput(
		const std::string& actionName,
		const std::vector<int>& keyboardKeys,
		const std::vector<int>& gamePadButtons,
		const std::vector<int>& mouseButtons)
	{
		InputAction action;
		action.keyboardKeys = keyboardKeys;
		action.gamePadButtons = gamePadButtons;
		action.mouseButtons = mouseButtons;

		// アクション名を小文字に変換して保存
		inputActions_[ToLower(actionName)] = action;

		if (useLogger_)
		{
			Logger::Output("Actionが登録されました [" + actionName + "]", Logger::Level::Engine);
		}
	}

	bool InputManager::IsPress(const std::string& actionName) const
	{

		// 小文字に変換して検索
		std::string lowerActionName = ToLower(actionName);
		auto it = inputActions_.find(lowerActionName);
		if (it == inputActions_.end())
		{
			return false;
		}

		bool result = CheckAnyPress(it->second);

		if (useLogger_) {

			// Pressが押されているとき
			if (result) {

				// まだログ出力していない場合のみ出力
				if (pressLoggedFlags_[lowerActionName] == false) {
					Logger::Output(" Actionが実行されました [Press]   : " + actionName, Logger::Level::Engine);
					pressLoggedFlags_[lowerActionName] = true;
				}
			} else {

				// 押されていないときはフラグをリセット
				pressLoggedFlags_[lowerActionName] = false;
			}
		}

		return result;
	}

	bool InputManager::IsTrigger(const std::string& actionName) const
	{

		// 小文字に変換して検索
		auto it = inputActions_.find(ToLower(actionName));
		if (it == inputActions_.end())
		{
			return false;
		}

		bool result = CheckAnyTrigger(it->second);
		if (result && useLogger_)
		{
			Logger::Output(" Actionが実行されました [Trigger] : " + actionName, Logger::Level::Engine);
		}
		return result;
	}

	bool InputManager::IsRelease(const std::string& actionName) const
	{

		// 小文字に変換して検索
		auto it = inputActions_.find(ToLower(actionName));
		if (it == inputActions_.end())
		{
			return false;
		}

		bool result = CheckAnyRelease(it->second);
		if (result && useLogger_)
		{
			Logger::Output("Actionが実行されました [Release] : " + actionName, Logger::Level::Engine);
		}
		return result;
	}

	bool InputManager::CheckAnyPress(const InputAction& action) const
	{

		// キーボードチェック
		for (int key : action.keyboardKeys)
		{
			if (keyboard_->IsPress(key))
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
		for (int key : action.keyboardKeys)
		{
			if (keyboard_->IsTrigger(key))
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
		for (int key : action.keyboardKeys)
		{
			if (keyboard_->IsRelease(key))
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
