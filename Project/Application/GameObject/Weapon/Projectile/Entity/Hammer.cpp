#include "Hammer.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Projectile {
	namespace {
		constexpr float kPivotHeightOffset = 2.0f;
		constexpr float kWindupDuration = 0.12f;
		constexpr float kRecoveryDuration = 0.12f;
		constexpr float kBaseModelScale = 1.0f;
		constexpr float kMinSizeRate = 0.1f;
		constexpr float kMinSwingSpeed = 0.1f;
		constexpr float kMinDirectionLengthSq = 0.000001f;
		constexpr float kMinSwingEndAngle = std::numbers::pi_v<float> * 0.5f;
		constexpr float kMaxSwingEndAngle = std::numbers::pi_v<float> * (2.0f / 3.0f);

		constexpr Vector3 kHitboxHalfSize = { 0.5f, 2.0f, 0.5f };
	}

	Hammer::~Hammer() {
		if (!objectName_.empty()) {

			// Colliderが参照する座標と有効状態を本体の破棄前に解除
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Hammer::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		state_ = State::Windup;
		stateTimer_.Start(kWindupDuration, false);
		swingAngle_ = 0.0f;
		isHitboxActive_ = false;
		hitEnemyIds_.clear();
		disappearsUponCollision_ = false;

		// 判定範囲の反転と振り下ろし時間のゼロ除算を防止
		if (!std::isfinite(sizeRate_)) {
			sizeRate_ = 1.0f;
		}
		sizeRate_ = std::max(sizeRate_, kMinSizeRate);
		if (!std::isfinite(moveSpeed_)) {
			moveSpeed_ = 7.0f;
		}
		const float swingSpeed = std::max(moveSpeed_, kMinSwingSpeed);
		const float modelScale = kBaseModelScale * sizeRate_;
		transform_.scale = { modelScale, modelScale, modelScale };
		transform_.translate = ownerPosition_ + Vector3{ 0.0f, kPivotHeightOffset, 0.0f };

		// 攻撃開始時の水平向きを固定して途中のTarget移動による軌道の反転を防止
		Vector3 toTarget = targetPosition_ - transform_.translate;
		const float targetHeightDifference = toTarget.y;
		toTarget.y = 0.0f;
		const float horizontalDistance = toTarget.Length();
		moveDirection_ = { 0.0f, 0.0f, 1.0f };
		if (toTarget.LengthSq() > kMinDirectionLengthSq) {
			moveDirection_ = toTarget.Normalized();
		}
		aimYaw_ = std::atan2(moveDirection_.x, moveDirection_.z);
		swingEndAngle_ = std::clamp(
			std::atan2(horizontalDistance, targetHeightDifference), kMinSwingEndAngle, kMaxSwingEndAngle);
		swingDuration_ = swingEndAngle_ / swingSpeed;
		lifeTimer_.Start(lifeTime_, false);

		model_ = MyModel::Create(objectName_, "Hammer", SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("white2x2");
		}
		hitbox_ = OBB{};
		UpdatePose();

		// 構え中と終了待機中の接触Damageを避けるため判定の有効状態を共有
		MyCollider::RegisterCollider(
			objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &hitboxPosition_,
			0.5f, nullptr, &isHitboxActive_);
	}

	void Hammer::Update(float deltaTime) {

		// Pause中と不正な時間入力では攻撃段階を進行させない制約
		if (isDead_ || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return;
		}
		lifeTimer_.Update(deltaTime);
		if (lifeTimer_.IsFinished()) {
			isHitboxActive_ = false;
			isDead_ = true;
			return;
		}

		// GameTimerの完了時に切り詰められる超過時間を次の攻撃段階へ引き継ぐため残り時間を退避
		const float remainingStateTime = stateTimer_.GetRemainingTime();
		stateTimer_.Update(deltaTime);
		switch (state_) {
		case State::Windup: {

			// 頭上の構えを見せた後、余った時間を振り下ろしへ引き継ぎ
			if (!stateTimer_.IsFinished()) {
				break;
			}
			stateTimer_.Start(swingDuration_, false);
			stateTimer_.Update(std::max(deltaTime - remainingStateTime, 0.0f));
			state_ = State::Swinging;
			[[fallthrough]];
		}
		case State::Swinging: {

			// 二次補間で先端の移動を加速し、根元を支点とした弧を生成
			isHitboxActive_ = true;
			const float progress = stateTimer_.GetProgress();
			swingAngle_ = swingEndAngle_ * progress * progress;
			if (stateTimer_.IsFinished()) {

				// 最終姿勢を一度衝突判定へ公開してから次Frameで攻撃判定を終了
				state_ = State::Recovery;
				stateTimer_.Start(kRecoveryDuration, false);
			}
			break;
		}
		case State::Recovery: {

			// 振り下ろし完了後は攻撃せず、最終姿勢の短い待機後に破棄
			isHitboxActive_ = false;
			if (stateTimer_.IsFinished()) {
				isDead_ = true;
				return;
			}
			break;
		}
		}

		UpdatePose();
		if (isHitboxActive_) {
			MyDebugLine::AddShape(hitbox_);
		}
	}

	void Hammer::SynchronizeOwnerPosition(const Vector3& ownerPosition) {
		ownerPosition_ = ownerPosition;
	}

	bool Hammer::HandleEnemyCollision(std::uint32_t enemyId, const Vector3* bounceTargetPosition) {
		(void)bounceTargetPosition;

		// 一振りの途中で離れて再接触しても同じEnemyへのDamageは一回に制限
		if (isDead_ || !isHitboxActive_) {
			return false;
		}
		return hitEnemyIds_.insert(enemyId).second;
	}

	void Hammer::UpdatePose() {
		transform_.translate = ownerPosition_ + Vector3{ 0.0f, kPivotHeightOffset, 0.0f };
		transform_.rotate = { swingAngle_, aimYaw_, 0.0f };
		UpdateHitbox();
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}
	}

	void Hammer::UpdateHitbox() {
		OBB& hitbox = std::get<OBB>(hitbox_);
		hitbox.rotate = transform_.rotate;
		hitbox.UpdateOrientation();
		const Vector3 halfSize = kHitboxHalfSize * transform_.scale;

		// 根元を支点に回転するため、ローカルY軸方向へ半分の長さだけ中心を補正
		hitboxPosition_ = transform_.translate + hitbox.orientation[1] * halfSize.y;
		hitbox.center = hitboxPosition_;
		hitbox.min = -halfSize;
		hitbox.max = halfSize;
	}
}
