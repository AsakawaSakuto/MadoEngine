#include "Axe.h"
#include "../../../Map/MapLimit.h"
#include <cmath>

namespace Projectile {

	Axe::~Axe() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::Destroy(objectName_);
		}
	}

	void Axe::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Test);
		//model_->SetTexture("AxeTexture");

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f * sizeRate_, 0.5f * sizeRate_, 0.5f * sizeRate_ };

		SetMoveDirectionTowards(targetPosition);

		AABB hitbox;
		float boxSize = 0.5f * sizeRate_;
		hitbox.min = { -boxSize, -boxSize, -boxSize };
		hitbox.max = { boxSize, boxSize, boxSize };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		isReductionStarted_ = false;
		disappearsUponCollision_ = false;
		
		reductionTimer_.Reset();
		startTimer_.Start(1.0f, false);
	}

	void Axe::Update(float deltaTime) {

		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox) || lifeTimer_.IsFinished()) {
			isDead_ = true;
			return;
		}

		if (startTimer_.IsActive()) {
			transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;
		}

		if (startTimer_.IsFinished()) {
			if (!lifeTimer_.IsActive()) {
				lifeTimer_.Start(lifeTime_, false);
			}
		}

		if (lifeTimer_.IsActive() && lifeTimer_.GetProgress() >= 0.9f && !isReductionStarted_) {
			isReductionStarted_ = true;
			reductionTimer_.Start(lifeTime_ * 0.1f, false);
		}

		transform_.rotate.y += 3.14f * deltaTime; // 回転速度を調整

		if (model_) {
			const float reductionProgress = isReductionStarted_ ? reductionTimer_.GetProgress() : 0.0f;
			const float scaleValue = 0.5f * sizeRate_ * (1.0f - reductionProgress); 
			transform_.scale = Easing::Lerp(Vector3{ scaleValue,scaleValue,scaleValue }, Vector3{ 0.0f,0.0f,0.0f }, reductionProgress,EaseType::Linear);
			model_->SetTransform(transform_);
		}

		lifeTimer_.Update(deltaTime);
		startTimer_.Update(deltaTime);
		reductionTimer_.Update(deltaTime);

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}
}
