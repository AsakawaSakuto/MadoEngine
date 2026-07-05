#include "Pistol.h"

namespace Projectile {
	namespace {
		constexpr float kMoveSpeed = 5.0f;
		constexpr float kDirectionEpsilon = 1e-5f;
	}
	
	void Pistol::Initialize(InitializeContext context) {
		model_ = MyModel::Create(context.projectileName + std::to_string(context.projectileCount), context.projectileName, SceneType::Test);

		ownerPosition = context.ownerPosition;
		targetPosition = context.targetPosition;

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		const Vector3 toTarget = targetPosition - ownerPosition;
		if (toTarget.LengthSq() > kDirectionEpsilon) {
			moveDirection_ = toTarget.Normalized();
		} else {
			moveDirection_ = { 0.0f, 0.0f, 0.0f };
		}

		if (model_) {
			model_->SetTransform(transform_);
		}

		AABB hitbox;
		hitbox.min = { -0.5f, -0.5f, -0.5f };
		hitbox.max = { 0.5f, 0.5f, 0.5f };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(context.projectileName + std::to_string(context.projectileCount), CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void Pistol::Update(float deltaTime) {

		transform_.translate += moveDirection_ * kMoveSpeed * deltaTime;

		if (model_) {
			model_->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}

}
