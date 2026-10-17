#include "SnowCrystal.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Projectile {
	namespace {
		constexpr float kDecelerationDuration = 0.6f;
		constexpr float kCatchRadius = 0.5f;
		constexpr float kRotationSpeed = std::numbers::pi_v<float> * 4.0f;
		constexpr float kBaseModelScale = 0.5f;
		constexpr float kBaseHitboxRadius = 0.5f;
		constexpr float kMinSizeRate = 0.1f;
		constexpr float kMinMoveSpeed = 0.1f;
		constexpr float kMinDirectionLengthSq = 0.000001f;
	}

	SnowCrystal::~SnowCrystal() {
		if (!objectName_.empty()) {

			// Colliderの座標参照を本体の破棄前に解除
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void SnowCrystal::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		transform_.translate = ownerPosition_;

		// ゼロ半径や逆方向への移動を防ぎ、往復できる正の速度を確保
		if (!std::isfinite(sizeRate_)) {
			sizeRate_ = 1.0f;
		}
		sizeRate_ = std::max(sizeRate_, kMinSizeRate);
		if (!std::isfinite(moveSpeed_)) {
			moveSpeed_ = 18.0f;
		}
		moveSpeed_ = std::max(moveSpeed_, kMinMoveSpeed);
		currentSpeed_ = moveSpeed_;
		const float modelScale = kBaseModelScale * sizeRate_;
		transform_.scale = { modelScale, modelScale, modelScale };

		// 往路は発射時の水平向きを固定し、Targetの移動による軌道変更を抑制
		Vector3 toTarget = targetPosition_ - transform_.translate;
		toTarget.y = 0.0f;
		moveDirection_ = { 0.0f, 0.0f, 1.0f };
		if (toTarget.LengthSq() > kMinDirectionLengthSq) {
			moveDirection_ = toTarget.Normalized();
		}
		lifeTimer_.Start(lifeTime_, false);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("white2x2");
			model->SetColor({ 0.5f, 0.85f, 1.0f, 1.0f });
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}

		Sphere hitbox;
		hitbox.radius = kBaseHitboxRadius * sizeRate_;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		// 敵への接触では消滅させず、往路と復路の両方で既存のDamage間隔を適用
		disappearsUponCollision_ = false;
	}

	void SnowCrystal::Update(float deltaTime) {

		// Pause中と不正な時間入力では往復移動と寿命を進行させない制約
		if (isDead_ || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return;
		}
		lifeTimer_.Update(deltaTime);
		if (lifeTimer_.IsFinished()) {
			isDead_ = true;
			return;
		}

		UpdateMovement(deltaTime);
		if (isDead_) {
			return;
		}

		// 移動方向とは独立した一定速度の回転でブーメランの往復を表現
		transform_.rotate.y = std::fmod(
			transform_.rotate.y + kRotationSpeed * deltaTime, std::numbers::pi_v<float> * 2.0f);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}
		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void SnowCrystal::SynchronizeOwnerPosition(const Vector3& ownerPosition) {
		ownerPosition_ = ownerPosition;
	}

	void SnowCrystal::UpdateMovement(float deltaTime) {
		const float deceleration = moveSpeed_ / kDecelerationDuration;
		const float previousSpeed = currentSpeed_;
		currentSpeed_ -= deceleration * deltaTime;

		// 速度がゼロをまたぐFrameも前進分を平均速度で積分し、時間刻みによる飛距離の差を抑制
		const float outboundTime = std::clamp(previousSpeed / deceleration, 0.0f, deltaTime);
		const float outboundDistance =
			(std::max(previousSpeed, 0.0f) + std::max(currentSpeed_, 0.0f)) * 0.5f * outboundTime;
		transform_.translate += moveDirection_ * outboundDistance;
		if (currentSpeed_ >= 0.0f) {
			return;
		}

		// 負の速度を復路の移動量へ変換し、折り返し後は同じ減速度で帰還方向へ加速
		const float returnTime = deltaTime - outboundTime;
		const float moveDistance =
			-(std::min(previousSpeed, 0.0f) + currentSpeed_) * 0.5f * returnTime;
		const Vector3 toOwner = ownerPosition_ - transform_.translate;
		const float distance = toOwner.Length();

		// 大きなdeltaTimeでもPlayerを通り越して往復し続けないよう到着を先に確定
		if (distance <= kCatchRadius || distance <= moveDistance) {
			transform_.translate = ownerPosition_;
			isDead_ = true;
			return;
		}

		// 発射地点ではなく移動中のPlayerを追跡し、斜面やジャンプの高低差にも追従
		moveDirection_ = toOwner / distance;
		transform_.translate += moveDirection_ * moveDistance;
	}
}
