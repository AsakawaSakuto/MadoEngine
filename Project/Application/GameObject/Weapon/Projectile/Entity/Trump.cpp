#include "Trump.h"
#include <algorithm>
#include <cmath>

namespace Projectile {

	Trump::~Trump() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Trump::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		baseDamage_ = damage_;

		transform_.translate = ownerPosition;
		const float safeSizeRate = std::isfinite(sizeRate_) ? std::max(sizeRate_, kMinSizeRate) : kMinSizeRate;
		const float size = kBaseSize * safeSizeRate;
		transform_.scale = { size, size, size };
		SetMoveDirectionTowards(targetPosition);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("Trump");
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}

		Sphere hitbox;
		hitbox.radius = size;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		const float safeLifeTime = std::isfinite(lifeTime_) ? std::max(lifeTime_, kMinLifeTime) : kMinLifeTime;

		lifeTimer_.Start(safeLifeTime, false);
	}

	void Trump::Update(float deltaTime) {
		lifeTimer_.Update(deltaTime);
		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;

		if (lifeTimer_.IsFinished() ||
			!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void Trump::OnEnemyHit() {

		// 貫通数が後から設定された場合も、前回の加算値を累積せず命中ごとに再抽選
		damage_ = baseDamage_ + static_cast<float>(
			MyRand::GetInt(kMinRandomDamageBonus, kMaxRandomDamageBonus));
	}
}
