#pragma once
#include "SceneType.h"
#include "Utility/GameTimer/GameTimer.h"

/// @brief シーン遷移時間の設定
struct SceneTransitionConfig {
	float exitDuration = 1.0f;  // 遷移元のシーン演出時間
	float enterDuration = 1.0f; // 遷移先のシーン演出時間
};

/// @brief シーン遷移の状態とEffect進行を管理
class SceneTransitionController final {
public:
	/// @brief シーン遷移要求の受付
	/// @param destinationSceneType 遷移先のシーン種別
	/// @return 遷移要求を受け付けた場合はtrue
	bool Request(SceneType destinationSceneType);

	/// @brief シーン遷移状態の更新
	/// @param deltaTime 経過時間
	void Update(float deltaTime);

	/// @brief シーン切替完了の通知
	/// @param currentSceneType 切替後のシーン種別
	/// @return 新シーン側の遷移演出を開始した場合はtrue
	bool NotifySceneChanged(SceneType currentSceneType);

	/// @brief シーン遷移状態のリセット
	void Reset();

	/// @brief シーン遷移中か確認
	/// @return 遷移演出の開始から完了までの間はtrue
	bool IsTransitioning() const;

	/// @brief Effect最大到達後のシーン切替待機中か確認
	/// @return シーン切替待機中の場合はtrue
	bool IsSceneChangeReady() const;

	/// @brief 遷移先のシーン種別を取得
	/// @return 遷移先のシーン種別、遷移要求がない場合はSceneType::None
	SceneType GetDestinationSceneType() const;

	/// @brief 現在段階の進行率を取得
	/// @return 0.0fから1.0fの進行率
	float GetProgress() const;

	/// @brief 遷移Effectへ適用する強度を取得
	/// @return 0.0fから1.0fのEffect強度
	float GetEffectProgress() const;

	/// @brief シーン遷移時間の設定
	/// @param config 遷移元と遷移先の演出時間設定
	void SetConfig(const SceneTransitionConfig& config);

	/// @brief シーン遷移時間の設定を取得
	/// @return 遷移元と遷移先の演出時間設定
	const SceneTransitionConfig& GetConfig() const;

private:
	/// @brief シーン遷移状態
	enum class State {
		Idle,
		PlayingExit,
		WaitingForSceneChange,
		PlayingEnter,
	};

	SceneTransitionConfig config_;
	GameTimer timer_;
	SceneType destinationSceneType_ = SceneType::None;
	State state_ = State::Idle;
};
