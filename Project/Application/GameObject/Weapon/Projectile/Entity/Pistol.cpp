#include "Pistol.h"
#include "../../../Map/MapLimit.h"
#include <cmath>

namespace Projectile {

	Pistol::~Pistol() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::Destroy(objectName_);
		}
	}
	
	void Pistol::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Test);

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		SetMoveDirectionTowards(targetPosition);

		AABB hitbox;
		hitbox.min = { -0.5f, -0.5f, -0.5f };
		hitbox.max = { 0.5f, 0.5f, 0.5f };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void Pistol::Update(float deltaTime) {

		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;

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
