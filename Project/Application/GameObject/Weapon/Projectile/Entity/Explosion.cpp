#include "Explosion.h"
#include <cmath>

namespace Projectile {
	namespace {
		constexpr float kMoveSpeed = 25.0f;
		constexpr float kDirectionEpsilon = 0.0001f;
	}

	Explosion::~Explosion() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
		}
	}

	void Explosion::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId) + "_Explosion";
		InitializeCommonProperties(context, objectName_);

		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		damage_ = context.damage * context.explotionDamageDecreaseRate;

		Sphere hitbox;
		hitbox.center = ownerPosition;
		hitbox.radius = context.explosionRadius;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		lifeTime_ = 0.1f;
		lifeTimer_.Start(lifeTime_, false);
	}

	void Explosion::Update(float deltaTime) {

		if (lifeTimer_.IsFinished()) {
			isDead_ = true;
		}

		lifeTimer_.Update(deltaTime);
		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}
}
