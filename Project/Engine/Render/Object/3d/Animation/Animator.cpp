#include "Animator.h"
#include "AnimationFunction.h"
#include <algorithm>
#include <cmath>

void Animator::Initialize(const AnimationSet* animationSet, const Skeleton& skeleton) {
	animationSet_ = animationSet;
	currentClip_ = nullptr;
	animationTime_ = 0.0f;
	blendElapsedTime_ = 0.0f;
	blendDuration_ = 0.0f;
	playbackSpeed_ = 1.0f;
	isFinished_ = false;
	CreateBindPose(skeleton, currentPose_);
	transitionSourcePose_ = currentPose_;
	targetPose_ = currentPose_;

	if (animationSet_ && animationSet_->GetDefaultClip()) {
		Play(animationSet_->GetDefaultClipName(), 0.0f, true);
	}
}

bool Animator::Play(std::string_view clipName, float blendDuration, bool restart) {
	if (!animationSet_) {
		return false;
	}

	const AnimationClip* nextClip = animationSet_->FindClip(clipName);
	if (!nextClip) {
		return false;
	}

	if (currentClip_ == nextClip && !restart) {
		return true;
	}

	// 遷移中の再切り替えでも見た目が跳ねないよう現在の合成済みPoseを始点として固定
	transitionSourcePose_ = currentPose_;
	currentClip_ = nextClip;
	animationTime_ = 0.0f;
	blendElapsedTime_ = 0.0f;
	blendDuration_ = (std::max)(0.0f, blendDuration < 0.0f ? nextClip->blendDuration : blendDuration);
	isFinished_ = false;
	return true;
}

void Animator::Update(float deltaTime, Skeleton& skeleton) {
	if (!currentClip_) {
		ApplyAnimationPose(skeleton, currentPose_);
		return;
	}

	const float safeDeltaTime = std::isfinite(deltaTime) ? (std::max)(0.0f, deltaTime) : 0.0f;
	const float clipPlaybackSpeed = (std::max)(0.0f, currentClip_->playbackSpeed);
	animationTime_ += safeDeltaTime * playbackSpeed_ * clipPlaybackSpeed;

	// Loop Clipは剰余で循環し、一回再生Clipは終端Poseを維持
	if (currentClip_->duration > 0.0f) {
		if (currentClip_->loop) {
			animationTime_ = std::fmod(animationTime_, currentClip_->duration);
			isFinished_ = false;
		} else if (animationTime_ >= currentClip_->duration) {
			animationTime_ = currentClip_->duration;
			isFinished_ = true;
		}
	} else {
		animationTime_ = 0.0f;
		isFinished_ = true;
	}

	SampleAnimationPose(skeleton, *currentClip_, animationTime_, targetPose_);
	blendElapsedTime_ = (std::min)(blendDuration_, blendElapsedTime_ + safeDeltaTime);
	if (blendDuration_ > 0.0f && blendElapsedTime_ < blendDuration_) {

		// 切り替え直前のPoseから新しいClipの現在PoseへJoint単位で補間
		BlendAnimationPose(
			transitionSourcePose_,
			targetPose_,
			blendElapsedTime_ / blendDuration_,
			currentPose_
		);
	} else {
		currentPose_ = targetPose_;
	}

	ApplyAnimationPose(skeleton, currentPose_);
}

void Animator::SetPlaybackSpeed(float playbackSpeed) {
	playbackSpeed_ = std::isfinite(playbackSpeed) ? (std::max)(0.0f, playbackSpeed) : 0.0f;
}

std::string_view Animator::GetCurrentClipName() const {
	return currentClip_ ? std::string_view(currentClip_->name) : std::string_view{};
}
