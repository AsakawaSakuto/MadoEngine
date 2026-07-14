#include "Pistol.h"
#include "../../../Map/MapLimit.h"
#include <cmath>

namespace Projectile {
	namespace {
		constexpr float kMoveSpeed = 25.0f;
		constexpr float kDirectionEpsilon = 0.0001f;
	}

	Pistol::~Pistol() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::Destroy(objectName_);
		}
	}
	
	void Pistol::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + std::to_string(context.projectileCount);
		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Test);

		ownerPosition = context.ownerPosition;
		targetPosition = context.targetPosition;

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		const Vector3 toTarget = targetPosition - ownerPosition;
		moveDirection_ = toTarget.Normalized();
		const float horizontalLength = std::sqrt(moveDirection_.x * moveDirection_.x + moveDirection_.z * moveDirection_.z);
		if (moveDirection_.LengthSq() > kDirectionEpsilon * kDirectionEpsilon) {
			transform_.rotate.x = std::atan2(-moveDirection_.y, horizontalLength);
			transform_.rotate.y = std::atan2(moveDirection_.x, moveDirection_.z);
			transform_.rotate.z = 0.0f;
		}

		damage_ = context.damage;

		AABB hitbox;
		hitbox.min = { -0.5f, -0.5f, -0.5f };
		hitbox.max = { 0.5f, 0.5f, 0.5f };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void Pistol::Update(float deltaTime) {

		transform_.translate += moveDirection_ * kMoveSpeed * deltaTime;

		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (model_) {
			model_->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}

}
