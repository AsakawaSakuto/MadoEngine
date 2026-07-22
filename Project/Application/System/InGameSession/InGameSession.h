#pragma once
#include "InGamePhase.h"

namespace System {

	class InGameSession {
	public:
		/// @brief ゲーム内セッションを初期化
		/// @param timeLimit ゲームプレイの制限時間（秒）
		void Initialize(float timeLimit);

		/// @brief ゲーム内セッションの状態を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief 武器アップグレード選択の待機状態を同期
		/// @param isActive 武器アップグレード選択中の場合はtrue
		void SetUpgradeSelectionActive(bool isActive);

		/// @brief ゲームプレイ中であるか確認
		/// @return ゲームプレイ中の場合はtrue
		bool IsPlaying() const { return currentPhase_ == InGamePhase::Playing; }

		/// @brief 現在のゲーム進行状態を取得
		/// @return 現在のゲーム進行状態
		InGamePhase GetCurrentPhase() const { return currentPhase_; }

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
		float timeLimit_ = 0.0f;
		float executionTime_ = 0.0f;
		bool isTimeUp_ = false;
	};

}