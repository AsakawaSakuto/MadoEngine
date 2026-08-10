#include "Explosion.h"
#include <cmath>

namespace Projectile {
	
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

		// 短い寿命中に範囲内の複数EnemyへHitできる衝突設定
		disappearsUponCollision_ = false;

		lifeTime_ = 0.1f;
		lifeTimer_.Start(lifeTime_, false);
	}

	void Explosion::Update(float deltaTime) {

		if (lifeTimer_.IsFinished()) {

			// 一Frame限りではなく衝突収集可能な短時間を維持してから破棄
			isDead_ = true;
		}

		lifeTimer_.Update(deltaTime);
		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}
}
