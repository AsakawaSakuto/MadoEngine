#include "EnemyTank.h"
#include "../EnemySettings.h"

namespace Enemy {

	void Tank::UpdateBehavior(float deltaTime) {
		MoveTowardPlayer(deltaTime);
	}

	std::string Tank::GetModelAssetName() const {
		return "enemy";
	}

	Vector3 Tank::GetModelScale() const {
		const float scaleMultiplier = Settings::GetInstance().GetTypeSettings(Data::Type::Tank).scaleMultiplier;
		return { scaleMultiplier, scaleMultiplier, scaleMultiplier };
	}

	Vector3 Tank::GetModelOffset() const {
		const float radius = Settings::GetInstance().GetTypeSettings(Data::Type::Tank).movementColliderRadius;
		return { 0.0f, -radius, 0.0f };
	}

	Sphere Tank::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = Settings::GetInstance().GetTypeSettings(Data::Type::Tank).movementColliderRadius;
		return sphere;
	}

	AABB Tank::CreateHitCollider() const {
		const TypeSettings& settings = Settings::GetInstance().GetTypeSettings(Data::Type::Tank);
		AABB aabb;
		aabb.min = settings.hitboxMin;
		aabb.max = settings.hitboxMax;
		return aabb;
	}

	bool Tank::ShouldDisappearOnPlayerCollision() const {
		return true;
	}

} // namespace Enemy
