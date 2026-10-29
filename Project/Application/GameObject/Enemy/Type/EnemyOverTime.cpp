#include "EnemyOverTime.h"
#include "../EnemySettings.h"

namespace Enemy {

	void OverTime::UpdateBehavior(float deltaTime) {
		MoveTowardPlayer(deltaTime);
	}

	std::string OverTime::GetModelAssetName() const {
		return "OverTimeEnemy";
	}

	Vector3 OverTime::GetModelScale() const {
		const float scaleMultiplier = Settings::GetInstance().GetTypeSettings(Data::Type::OverTime).scaleMultiplier;
		return { scaleMultiplier, scaleMultiplier, scaleMultiplier };
	}

	Vector3 OverTime::GetModelOffset() const {
		const float radius = Settings::GetInstance().GetTypeSettings(Data::Type::OverTime).movementColliderRadius;
		return { 0.0f, -radius, 0.0f };
	}

	Sphere OverTime::CreateMovementCollider() const {
		Sphere sphere;
		sphere.radius = Settings::GetInstance().GetTypeSettings(Data::Type::OverTime).movementColliderRadius;
		return sphere;
	}

	AABB OverTime::CreateHitCollider() const {
		const TypeSettings& settings = Settings::GetInstance().GetTypeSettings(Data::Type::OverTime);
		AABB aabb;
		aabb.min = settings.hitboxMin;
		aabb.max = settings.hitboxMax;
		return aabb;
	}
} // namespace Enemy
