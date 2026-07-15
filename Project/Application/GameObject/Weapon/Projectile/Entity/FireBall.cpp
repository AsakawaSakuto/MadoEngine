#include "FireBall.h"
#include "../../../Map/MapLimit.h"
#include "../ProjectileManager.h"
#include <cmath>

namespace Projectile {

	FireBall::~FireBall() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::Destroy(objectName_);
		}
	}

	void FireBall::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Test);

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		const Vector3 toTarget = targetPosition - ownerPosition;
		moveDirection_ = toTarget.Normalized();
		const float horizontalLength = std::sqrt(moveDirection_.x * moveDirection_.x + moveDirection_.z * moveDirection_.z);
		transform_.rotate.x = std::atan2(-moveDirection_.y, horizontalLength);
		transform_.rotate.y = std::atan2(moveDirection_.x, moveDirection_.z);
		transform_.rotate.z = 0.0f;

		AABB hitbox;
		hitbox.min = { -0.5f, -0.5f, -0.5f };
		hitbox.max = { 0.5f, 0.5f, 0.5f };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void FireBall::Update(float deltaTime) {

		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;

		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			Projectile::InitializeDesc context;
			context.projectileName = objectName_;
			context.ownerPosition = transform_.translate;
			context.damage = damage_;
			context.explotionDamageDecreaseRate = 0.75f;
			context.explosionRadius = 1.0f;
			Projectile::Manager::GetInstance().AddProjectile(Projectile::Type::Explosion, context);
			isDead_ = true;
			return;
		}

		if (model_) {
			model_->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}

}
