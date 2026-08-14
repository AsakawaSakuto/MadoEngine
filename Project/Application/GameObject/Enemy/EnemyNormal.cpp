#include "EnemyNormal.h"

namespace Enemy {

	void Normal::UpdateBehavior(float deltaTime) {
		MoveTowardPlayer(deltaTime);
	}

	std::string Normal::GetModelAssetName() const {
		return "enemy";
	}

	Vector3 Normal::GetModelScale() const {
		return { 0.5f, 0.5f, 0.5f };
	}

	Vector3 Normal::GetModelOffset() const {
		return { 0.0f, -0.5f, 0.0f };
	}

	Sphere Normal::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = 0.5f;
		return sphere;
	}

	AABB Normal::CreateHitCollider() const {
		AABB aabb;
		aabb.min = { -0.5f, 0.0f, -0.5f };
		aabb.max = { 0.5f, 2.0f, 0.5f };
		return aabb;
	}

	bool Normal::ShouldDisappearOnPlayerCollision() const {
		return true;
	}

} // namespace Enemy
