#include "EnemyTank.h"

namespace Enemy {

	void Tank::UpdateBehavior(float deltaTime) {
		MoveTowardPlayer(deltaTime);
	}

	std::string Tank::GetModelAssetName() const {
		return "enemy";
	}

	Vector3 Tank::GetModelScale() const {
		return { 0.8f, 0.8f, 0.8f };
	}

	Vector3 Tank::GetModelOffset() const {
		return { 0.0f, -0.8f, 0.0f };
	}

	Sphere Tank::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = 0.8f;
		return sphere;
	}

	AABB Tank::CreateHitCollider() const {
		AABB aabb;
		aabb.min = { -0.8f, 0.0f, -0.8f };
		aabb.max = { 0.8f, 3.2f, 0.8f };
		return aabb;
	}

	bool Tank::ShouldDisappearOnPlayerCollision() const {
		return true;
	}

} // namespace Enemy
