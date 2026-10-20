#pragma once
#include "InGamePhase.h"
#include "Utility/GameTimer/GameTimer.h"

namespace System {

	class InGameSession {
	public:
		/// @brief ゲーム内セッションを初期化
		/// @param timeLimit ゲームプレイの制限時間（秒）
		void Initialize(float timeLimit);

		/// @brief ゲーム内セッションの状態を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param canStart シーン遷移などの開始前条件を満たす場合はtrue
		/// @return ゲーム進行用の経過時間、開始待機と停止中は0秒
		float Update(float deltaTime, bool canStart = true);

		/// @brief 武器アップグレード選択の待機状態を同期
		/// @param isActive 武器アップグレード選択中の場合はtrue
		void SetUpgradeSelectionActive(bool isActive);

		/// @brief レリック獲得の待機状態を同期
		/// @param isActive レリック獲得待ちの場合はtrue
		void SetItemAcquisitionActive(bool isActive);

		/// @brief ゲーム進行をGameOver状態へ変更
		void SetGameOver();

		/// @brief ゲームプレイ中であるか確認
		/// @return ゲームプレイ中の場合はtrue
		bool IsPlaying() const { return currentPhase_ == InGamePhase::Playing; }

		/// @brief 武器アップグレード選択の待機状態であるか確認
		/// @return 武器アップグレード選択の待機状態の場合はtrue
		bool IsWaitingUpgradeSelection() const { return currentPhase_ == InGamePhase::WaitingUpgrade; }

		/// @brief レリック獲得の待機状態であるか確認
		/// @return レリック獲得の待機状態の場合はtrue
		bool IsWaitingItemAcquisition() const { return currentPhase_ == InGamePhase::WaitingGetItem; }

		/// @brief 現在のゲーム進行状態を取得
		/// @return 現在のゲーム進行状態
		InGamePhase GetCurrentPhase() const { return currentPhase_; }

		/// @brief ゲーム開始待機の進行率を取得
		/// @return 開始前は0.0f、完了後は1.0fの進行率
		float GetStartDelayProgress() const;

		/// @brief ゲームプレイの累積実行時間を取得
		/// @return ゲームプレイ中に経過した秒数
		float GetExecutionTime() const { return executionTime_; }

		/// @brief ゲームプレイの残り時間を取得
		/// @return 0秒以上に補正された残り時間
		float GetRemainingTime() const;

		/// @brief 制限時間に到達したか確認
		/// @return 制限時間に到達した場合はtrue
		bool IsTimeUp() const { return isTimeUp_; }

	private:
		InGamePhase currentPhase_ = InGamePhase::Starting;
		GameTimer gameStartDelayTimer_;
		float timeLimit_ = 0.0f;
		float executionTime_ = 0.0f;
		bool isTimeUp_ = false;
	};

}
