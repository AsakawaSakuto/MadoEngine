#include "EnemyRunner.h"

namespace Enemy {

	void Runner::UpdateBehavior(float deltaTime) {
		MoveTowardPlayer(deltaTime);
	}

	std::string Runner::GetModelAssetName() const {
		return "enemy";
	}

	Vector3 Runner::GetModelScale() const {
		return { 0.35f, 0.35f, 0.35f };
	}

	Vector3 Runner::GetModelOffset() const {
		return { 0.0f, -0.35f, 0.0f };
	}

	Sphere Runner::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = 0.35f;
		return sphere;
	}

	AABB Runner::CreateHitCollider() const {
		AABB aabb;
		aabb.min = { -0.35f, 0.0f, -0.35f };
		aabb.max = { 0.35f, 1.4f, 0.35f };
		return aabb;
	}

	bool Runner::ShouldDisappearOnPlayerCollision() const {
		return true;
	}

} // namespace Enemy
