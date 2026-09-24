#include "Orb.h"
#include <algorithm>
#include <cmath>

namespace Projectile {

	Orb::~Orb() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Orb::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		startPosition_ = ownerPosition;
		transform_.translate = startPosition_;
		const float safeSizeRate = std::isfinite(sizeRate_) ? std::max(sizeRate_, kMinSizeRate) : kMinSizeRate;
		const float modelScale = kBaseModelScale * safeSizeRate;
		transform_.scale = { modelScale, modelScale, modelScale };

		const Vector3 toTarget = targetPosition - startPosition_;
		const float distance = toTarget.Length();
		const float safeMoveSpeed = std::isfinite(moveSpeed_) ? std::max(std::fabs(moveSpeed_), kMinMoveSpeed) : kMinMoveSpeed;

		flightDuration_ = std::max(distance / safeMoveSpeed, kMinFlightDuration);

		const float horizontalDistance = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
		arcHeight_ = std::clamp(horizontalDistance * kArcHeightDistanceRate, kMinArcHeight, kMaxArcHeight);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("white2x2");
			model->SetColor({ 0.25f, 0.65f, 1.0f, 1.0f });
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}

		Sphere hitbox;
		hitbox.radius = kBaseHitboxRadius * safeSizeRate;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		flightElapsedTime_ = 0.0f;
		arrivalElapsedTime_ = 0.0f;
		hasArrived_ = false;
	}

	void Orb::Update(float deltaTime) {
		const float safeDeltaTime = std::isfinite(deltaTime)
			? std::max(deltaTime, 0.0f)
			: 0.0f;

		if (!hasArrived_) {
			flightElapsedTime_ += safeDeltaTime;
			const float progress = std::clamp(
				flightElapsedTime_ / flightDuration_, 0.0f, 1.0f);

			// 直線補間へ4t(1-t)の高さを加え、両端を固定した上向きの放物線を作る
			transform_.translate = Math::Lerp(startPosition_, targetPosition, progress);
			transform_.translate.y += 4.0f * arcHeight_ * progress * (1.0f - progress);
			transform_.rotate.y += kRotationSpeed * safeDeltaTime;

			if (progress >= 1.0f) {
				hasArrived_ = true;
			}
		} else {

			// Projectile更新より前に行われるEnemy衝突判定へ着地点を一時的に公開
			arrivalElapsedTime_ += safeDeltaTime;
			if (arrivalElapsedTime_ >= kArrivalHoldDuration) {
				isDead_ = true;
				return;
			}
		}

		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}
}
