#include "Explotion.h"
#include <cmath>

namespace Projectile {
	namespace {
		constexpr float kMoveSpeed = 25.0f;
		constexpr float kDirectionEpsilon = 0.0001f;
	}

	Explotion::~Explotion() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::Destroy(objectName_);
		}
	}

	void Explotion::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + std::to_string(context.projectileCount) + "Explotion";
		
		ownerPosition = context.ownerPosition;
		targetPosition = context.targetPosition;

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		damage_ = context.damage * context.explotionDamageDecreaseRate;

		Sphere hitbox;
		hitbox.center = ownerPosition;
		hitbox.radius = context.explosionRadius;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void Explotion::Update(float deltaTime) {

		deadTime_ += deltaTime;

		if (deadTime_ > deadTimeLimit_) {
			isDead_ = true;
		}

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}
}