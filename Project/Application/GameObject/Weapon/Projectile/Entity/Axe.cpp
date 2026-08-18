#include "Axe.h"
#include "../../../Map/MapLimit.h"
#include <cmath>

namespace Projectile {

	Axe::~Axe() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Axe::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f * sizeRate_, 0.5f * sizeRate_, 0.5f * sizeRate_ };

		SetMoveDirectionTowards(targetPosition, true);

		AABB hitbox;
		float boxSize = 0.5f * sizeRate_;
		hitbox.min = { -boxSize, -boxSize, -boxSize };
		hitbox.max = { boxSize, boxSize, boxSize };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		isReductionStarted_ = false;
		disappearsUponCollision_ = false;

		// 一秒間の直進後に滞留寿命へ移行する二段階Timer
		reductionTimer_.Reset();
		startTimer_.Start(1.0f, false);
	}

	void Axe::Update(float deltaTime) {

		// Map外への移動または滞留寿命の終了でProjectileを破棄
		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox) || lifeTimer_.IsFinished()) {
			isDead_ = true;
			return;
		}

		if (startTimer_.IsActive()) {

			// 初期移動期間だけ前方へ進み、終了後はその場で回転
			transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;
		}

		if (startTimer_.IsFinished()) {
			if (!lifeTimer_.IsActive()) {
				lifeTimer_.Start(lifeTime_, false);
			}
		}

		if (lifeTimer_.IsActive() && lifeTimer_.GetProgress() >= 0.9f && !isReductionStarted_) {

			// 寿命末尾の一割を縮小演出へ割り当て
			isReductionStarted_ = true;
			reductionTimer_.Start(lifeTime_ * 0.1f, false);
		}

		// 滞留中も武器らしい視認性を保つ一定速度の回転
		transform_.rotate.y += 3.14f * deltaTime;

		if (Model* model = MyModel::TryGet(model_)) {

			// 消滅直前だけScaleを連続的に縮小して突然の非表示を回避
			const float reductionProgress = isReductionStarted_ ? reductionTimer_.GetProgress() : 0.0f;
			const float scaleValue = 0.5f * sizeRate_ * (1.0f - reductionProgress);
			transform_.scale = Easing::Lerp(Vector3{ scaleValue,scaleValue,scaleValue }, Vector3{ 0.0f,0.0f,0.0f }, reductionProgress, EaseType::Linear);
			model->SetTransform(transform_);
		}

		lifeTimer_.Update(deltaTime);
		startTimer_.Update(deltaTime);
		reductionTimer_.Update(deltaTime);

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}
}
