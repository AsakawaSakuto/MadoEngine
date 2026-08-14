#include "EnemySpeed.h"

namespace Enemy {

	void Speed::UpdateBehavior(float deltaTime) {
		constexpr float kSpeedMultiplier = 1.6f;
		MoveTowardPlayer(deltaTime, kSpeedMultiplier);
	}

	std::string Speed::GetModelAssetName() const {
		return "enemy";
	}

	Vector3 Speed::GetModelScale() const {
		return { 0.4f, 0.4f, 0.4f };
	}

	Vector3 Speed::GetModelOffset() const {
		return { 0.0f, -0.4f, 0.0f };
	}

	Sphere Speed::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = 0.4f;
		return sphere;
	}

	AABB Speed::CreateHitCollider() const {
		AABB aabb;
		aabb.min = { -0.4f, 0.0f, -0.4f };
		aabb.max = { 0.4f, 1.6f, 0.4f };
		return aabb;
	}

	bool Speed::ShouldDisappearOnPlayerCollision() const {
		return true;
	}

} // namespace Enemy
