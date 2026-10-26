#include "InGameSession.h"
#include "Input/MyInput.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kGameStartDelayDuration = 1.0f; // 開始前条件の成立後に挟む待機時間（秒）
	constexpr float kPauseMenuStickInputThreshold = 0.5f;
	constexpr float kPauseMenuStickNeutralThreshold = 0.2f;
}

namespace System {

	void InGameSession::Initialize(float timeLimit) {
		currentPhase_ = InGamePhase::Starting;
		selectedPauseMenuItem_ = PauseMenuItem::Continue;
		pauseMenuDecision_.reset();
		gameStartDelayTimer_.Reset();
		timeLimit_ = (std::max)(0.0f, timeLimit);
		executionTime_ = 0.0f;
		isTimeUp_ = false;
		isPauseMenuStickReady_ = false;
	}

	float InGameSession::Update(float deltaTime, bool canStart) {

		// 遷移演出中は開始待機だけでなくPause解除とメニュー操作および制限時間の進行も停止
		if (!canStart) {
			isPauseMenuStickReady_ = false;
			return 0.0f;
		}

		// 開始前条件の成立後も待機時間を確保し、Playingへの移行をSession内で管理
		switch (currentPhase_) {
		case InGamePhase::Starting:
			if (!gameStartDelayTimer_.IsActive()) {

				// 条件成立Frameの時間を待機へ持ち込まないよう開始だけを実行
				gameStartDelayTimer_.Start(kGameStartDelayDuration);
				return 0.0f;
			}
			gameStartDelayTimer_.Update(deltaTime);
			if (!gameStartDelayTimer_.IsFinished()) {
				return 0.0f;
			}
			currentPhase_ = InGamePhase::Playing;

			// 待機終了Frameの時間を制限時間とGameObjectへ重複加算しない開始境界
			return 0.0f;
		case InGamePhase::Playing:
			if (MyInput::Trigger("Pause")) {

				// 開いたFrameの方向入力と決定を持ち込まず、毎回Continueから選択を開始
				currentPhase_ = InGamePhase::Paused;
				selectedPauseMenuItem_ = PauseMenuItem::Continue;
				pauseMenuDecision_.reset();

				// 移動中の傾きを選択へ持ち込まず、表示開始時に中立の場合だけ次の傾きを受付
				isPauseMenuStickReady_ = false;
				GetPauseMenuStickTrigger();
			}
			break;
		case InGamePhase::Paused:
			UpdatePauseMenu();

			// Continueの決定入力を同じFrameのゲーム操作へ渡さない再開境界
			return 0.0f;
		case InGamePhase::WaitingUpgrade:
		case InGamePhase::WaitingGetItem:
		case InGamePhase::GameOver:
			break;
		}

		if (!IsPlaying()) {
			return 0.0f;
		}

		// 制限時間とGameObjectへ同じ経過時間を適用して開始待機や停止との時間差を防止
		const float gameDeltaTime = (std::max)(0.0f, deltaTime);
		executionTime_ = (std::min)(timeLimit_, executionTime_ + gameDeltaTime);

		if (executionTime_ >= timeLimit_) {
			isTimeUp_ = true;

			// GameOverへの遷移判断は通知を受け取る呼び出し側へ委譲
		}
		return gameDeltaTime;
	}

	void InGameSession::UpdatePauseMenu() {

		// Pauseキーによる既存の再開操作を方向入力と決定より優先
		if (MyInput::Trigger("Pause")) {
			currentPhase_ = InGamePhase::Playing;
			return;
		}

		// 上端と下端を循環し、一Frameでの選択移動を一項目へ制限
		constexpr int kMenuItemCount = static_cast<int>(PauseMenuItem::Quit) + 1;
		int selectedIndex = static_cast<int>(selectedPauseMenuItem_);
		const int stickSelectionInput = GetPauseMenuStickTrigger();
		if (MyInput::Trigger("Up") || stickSelectionInput < 0) {
			selectedIndex = (selectedIndex + kMenuItemCount - 1) % kMenuItemCount;
		} else if (MyInput::Trigger("Down") || stickSelectionInput > 0) {
			selectedIndex = (selectedIndex + 1) % kMenuItemCount;
		}
		selectedPauseMenuItem_ = static_cast<PauseMenuItem>(selectedIndex);
		if (!MyInput::Trigger("Decision")) {
			return;
		}

		// Scene遷移は通知を受け取るGameSceneへ委譲し、ContinueだけをSession内で解決
		pauseMenuDecision_ = selectedPauseMenuItem_;
		if (selectedPauseMenuItem_ == PauseMenuItem::Continue) {
			currentPhase_ = InGamePhase::Playing;
		}
	}

	int InGameSession::GetPauseMenuStickTrigger() {
		const auto* gamePad = MyInput::GetGamePad();
		if (!gamePad || !gamePad->IsConnected()) {

			// 接続直後の傾きを新しい選択と誤認しないよう再度の中立復帰を要求
			isPauseMenuStickReady_ = false;
			return 0;
		}
		const Vector2 stick = gamePad->GetLeftStick();

		// 狭い中立範囲でのみ再入力を許可し、閾値付近の揺れや直接の切り返しによる連続移動を防止
		if (std::abs(stick.x) <= kPauseMenuStickNeutralThreshold &&
			std::abs(stick.y) <= kPauseMenuStickNeutralThreshold) {
			isPauseMenuStickReady_ = true;
			return 0;
		}
		if (!isPauseMenuStickReady_) {
			return 0;
		}

		// GamePadのY正方向を上、負方向を下としてメニューの循環方向へ変換
		if (stick.y >= kPauseMenuStickInputThreshold) {
			isPauseMenuStickReady_ = false;
			return -1;
		}
		if (stick.y <= -kPauseMenuStickInputThreshold) {
			isPauseMenuStickReady_ = false;
			return 1;
		}
		return 0;
	}

	std::optional<PauseMenuItem> InGameSession::ConsumePauseMenuDecision() {
		const std::optional<PauseMenuItem> decision = pauseMenuDecision_;
		pauseMenuDecision_.reset();
		return decision;
	}

	float InGameSession::GetStartDelayProgress() const {
		if (!gameStartDelayTimer_.IsActive() && !gameStartDelayTimer_.IsFinished()) {
			return 0.0f;
		}
		return gameStartDelayTimer_.GetProgress();
	}

	float InGameSession::GetRemainingTime() const {
		return (std::max)(0.0f, timeLimit_ - executionTime_);
	}

	void InGameSession::SetUpgradeSelectionActive(bool isActive) {

		// Upgrade選択による停止を通常プレイ中からの遷移に限定
		if (isActive && currentPhase_ == InGamePhase::Playing) {
			currentPhase_ = InGamePhase::WaitingUpgrade;
			return;
		}

		// 他の待機理由を誤って解除しないようWaitingUpgradeだけを再開
		if (!isActive && currentPhase_ == InGamePhase::WaitingUpgrade) {
			currentPhase_ = InGamePhase::Playing;
		}
	}

	void InGameSession::SetItemAcquisitionActive(bool isActive) {

		// 宝箱報酬を武器強化より優先して通常プレイから獲得待ちへ遷移
		if (isActive && currentPhase_ == InGamePhase::Playing) {
			currentPhase_ = InGamePhase::WaitingGetItem;
			return;
		}

		// 他の停止理由を解除しないようレリック獲得待ちだけを再開
		if (!isActive && currentPhase_ == InGamePhase::WaitingGetItem) {
			currentPhase_ = InGamePhase::Playing;
		}
	}

	void InGameSession::SetGameOver() {
		currentPhase_ = InGamePhase::GameOver;
	}

} // namespace System
