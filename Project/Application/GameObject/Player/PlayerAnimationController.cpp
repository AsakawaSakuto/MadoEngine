#include "PlayerAnimationController.h"
#include "Render/Object/3d/Model/Model.h"
#include "Utility/Logger/Logger.h"
#include <string>

namespace Player {

	void AnimationController::Initialize(Model& model) {
		currentMotion_.reset();
		isJumpAnimationActive_ = false;
		isLandingAnimationActive_ = false;
		Update(Motion::Idle, false, true, false, model);
	}

	void AnimationController::Update(Motion motion, bool isMoving, bool isGrounded, bool isJumpStarted, Model& model) {
		if (motion == Motion::Climbing) {
			model.SetAnimationPlaybackSpeed(1.0f);
			isJumpAnimationActive_ = false;
			isLandingAnimationActive_ = false;

			// 壁登り中は空中遷移の残存状態よりClimbing Clipを優先
			if (!currentMotion_ || *currentMotion_ != Motion::Climbing) {
				if (!model.PlayAnimation("Climbing", GetBlendDuration(Motion::Climbing))) {
					Logger::Output("[Application] Player用AnimationClipが見つかりません: Climbing", Logger::Level::Warning);
				}
				currentMotion_ = Motion::Climbing;
			}
			return;
		}

		if (isJumpStarted) {

			// 成立したJump入力を最優先し、空中での再入力も先頭Poseから即時再生
			model.SetAnimationPlaybackSpeed(1.0f);
			if (model.PlayAnimation("Jump", 0.0f, true)) {
				currentMotion_ = Motion::Jump;
				isJumpAnimationActive_ = true;
				isLandingAnimationActive_ = false;
				return;
			}

			Logger::Output("[Application] Player用AnimationClipが見つかりません: Jump", Logger::Level::Warning);
			isJumpAnimationActive_ = false;
		}

		if (isJumpAnimationActive_) {
			if (!isGrounded) {

				// 非LoopのJump Clipは終端到達後も着地まで最終Poseを維持
				model.SetAnimationPlaybackSpeed(model.IsAnimationFinished() ? 0.0f : 1.0f);
				currentMotion_ = Motion::Jump;
				return;
			}

			model.SetAnimationPlaybackSpeed(1.0f);
			isJumpAnimationActive_ = false;
			currentMotion_.reset();
			if (model.HasAnimationClip("Landing") && model.PlayAnimation("Landing", 0.08f, true)) {

				// 接地後は一回再生のLandingが終わるまで移動Animationへの遷移を待機
				currentMotion_ = motion;
				isLandingAnimationActive_ = true;
				return;
			}
		}

		if (isLandingAnimationActive_) {
			model.SetAnimationPlaybackSpeed(1.0f);
			currentMotion_ = motion;
			if (!model.IsAnimationFinished()) {
				return;
			}

			// Landing Clip終端で現在の移動Animation選択へ復帰
			isLandingAnimationActive_ = false;
			currentMotion_.reset();
		}

		// Crouching Poseを維持したまま水平移動が止まった時点でClipの時刻だけ停止
		model.SetAnimationPlaybackSpeed(motion == Motion::Crouching && !isMoving ? 0.0f : 1.0f);

		if (currentMotion_ && *currentMotion_ == motion) {
			return;
		}

		const char* clipName = GetClipName(motion);
		if (!model.PlayAnimation(clipName, GetBlendDuration(motion))) {
			Logger::Output(
				"[Application] Player用AnimationClipが見つかりません: " + std::string(clipName),
				Logger::Level::Warning
			);
			currentMotion_ = motion;
			return;
		}

		currentMotion_ = motion;
		isJumpAnimationActive_ = motion == Motion::Jump && !isGrounded;
	}

	const char* AnimationController::GetClipName(Motion motion) {
		switch (motion) {
		case Motion::Idle:
			return "Idle";
		case Motion::Walk:
			return "Walk";
		case Motion::Crouching:
			return "Crouching";
		case Motion::Jump:
			return "Jump";
		case Motion::Climbing:
			return "Climbing";
		default:
			return "Idle";
		}
	}

	float AnimationController::GetBlendDuration(Motion motion) {
		switch (motion) {
		case Motion::Jump:
			return 0.0f;
		case Motion::Crouching:
			return 0.12f;
		case Motion::Climbing:
			return 0.12f;
		case Motion::Idle:
		case Motion::Walk:
		default:
			return 0.15f;
		}
	}
}
