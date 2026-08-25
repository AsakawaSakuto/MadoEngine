#include "EnemyNormal.h"
#include "../EnemySettings.h"

namespace Enemy {

	void Normal::UpdateBehavior(float deltaTime) {
		MoveTowardPlayer(deltaTime);
	}

	std::string Normal::GetModelAssetName() const {
		return "enemy";
	}

	Vector3 Normal::GetModelScale() const {
		const float scaleMultiplier = Settings::GetInstance().GetTypeSettings(Data::Type::Normal).scaleMultiplier;
		return { scaleMultiplier, scaleMultiplier, scaleMultiplier };
	}

	Vector3 Normal::GetModelOffset() const {
		const float radius = Settings::GetInstance().GetTypeSettings(Data::Type::Normal).movementColliderRadius;
		return { 0.0f, -radius, 0.0f };
	}

	Sphere Normal::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = Settings::GetInstance().GetTypeSettings(Data::Type::Normal).movementColliderRadius;
		return sphere;
	}

	AABB Normal::CreateHitCollider() const {
		const TypeSettings& settings = Settings::GetInstance().GetTypeSettings(Data::Type::Normal);
		AABB aabb;
		aabb.min = settings.hitboxMin;
		aabb.max = settings.hitboxMax;
		return aabb;
	}

	bool Normal::ShouldDisappearOnPlayerCollision() const {
		return true;
	}

} // namespace Enemy
