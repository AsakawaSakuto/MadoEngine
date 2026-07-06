#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "ProjectileStatus.h"
#include "../WeaponStatus.h"
#include <vector>
#include <memory>
#include <string>

namespace Projectile {

	class IProjectile {
	public:
		virtual void Initialize(InitializeDesc context) = 0;

		virtual void Update(float deltaTime) = 0;

	protected:

		Transform3D transform_;
		ColliderShape hitbox_;

		Vector3 ownerPosition = { 0.0f, 0.0f, 0.0f };
		Vector3 targetPosition = { 0.0f, 0.0f, 0.0f };
	};
}