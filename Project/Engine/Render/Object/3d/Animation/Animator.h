#pragma once
#include "AnimationSet.h"
#include <string_view>

/// @brief SkeletonごとのAnimation再生状態とPose遷移を管理するクラス
class Animator {
public:

	/// @brief 再生対象のAnimationSetとSkeletonを設定
	/// @param animationSet 再生対象のAnimationSet
	/// @param skeleton Pose生成の基準になるSkeleton
	void Initialize(const AnimationSet* animationSet, const Skeleton& skeleton);

	/// @brief 指定したAnimationClipへ遷移
	/// @param clipName 再生するAnimationClip名
	/// @param blendDuration 遷移時間、負数の場合はClipの標準値
	/// @param restart 同じClipでも先頭から再生する場合はtrue
	/// @return 再生を開始できた場合はtrue
	bool Play(std::string_view clipName, float blendDuration = -1.0f, bool restart = false);

	/// @brief Animation再生時刻とSkeleton Poseを更新
	/// @param deltaTime 前フレームからの経過時間
	/// @param skeleton Pose反映対象のSkeleton
	void Update(float deltaTime, Skeleton& skeleton);

	/// @brief 再生速度倍率を設定
	/// @param playbackSpeed 再生速度倍率
	void SetPlaybackSpeed(float playbackSpeed);

	/// @brief 現在再生中のAnimationClip名を取得
	/// @return 現在再生中のAnimationClip名
	std::string_view GetCurrentClipName() const;

	/// @brief 現在のAnimationClipが終端へ到達したか判定
	/// @return 終端へ到達した場合はtrue
	bool IsFinished() const { return isFinished_; }

	/// @brief 再生可能なAnimationClipを保持しているか判定
	/// @return 再生可能なAnimationClipを保持している場合はtrue
	bool HasAnimation() const { return animationSet_ && !animationSet_->IsEmpty(); }

private:
	const AnimationSet* animationSet_ = nullptr;
	const AnimationClip* currentClip_ = nullptr;
	float animationTime_ = 0.0f;
	float blendElapsedTime_ = 0.0f;
	float blendDuration_ = 0.0f;
	float playbackSpeed_ = 1.0f;
	bool isFinished_ = false;
	AnimationPose currentPose_;
	AnimationPose transitionSourcePose_;
	AnimationPose targetPose_;
};
