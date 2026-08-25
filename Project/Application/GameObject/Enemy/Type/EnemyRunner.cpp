#include "EnemyRunner.h"
#include "../EnemySettings.h"

namespace Enemy {

	void Runner::UpdateBehavior(float deltaTime) {
		MoveTowardPlayer(deltaTime);
	}

	std::string Runner::GetModelAssetName() const {
		return "enemy";
	}

	Vector3 Runner::GetModelScale() const {
		const float scaleMultiplier = Settings::GetInstance().GetTypeSettings(Data::Type::Runner).scaleMultiplier;
		return { scaleMultiplier, scaleMultiplier, scaleMultiplier };
	}

	Vector3 Runner::GetModelOffset() const {
		const float radius = Settings::GetInstance().GetTypeSettings(Data::Type::Runner).movementColliderRadius;
		return { 0.0f, -radius, 0.0f };
	}

	Sphere Runner::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = Settings::GetInstance().GetTypeSettings(Data::Type::Runner).movementColliderRadius;
		return sphere;
	}

	AABB Runner::CreateHitCollider() const {
		const TypeSettings& settings = Settings::GetInstance().GetTypeSettings(Data::Type::Runner);
		AABB aabb;
		aabb.min = settings.hitboxMin;
		aabb.max = settings.hitboxMax;
		return aabb;
	}

	bool Runner::ShouldDisappearOnPlayerCollision() const {
		return true;
	}

} // namespace Enemy
