#include "Pistol.h"

namespace Projectile {
	
	void Pistol::Initialize(const std::string& entityName, int projectileCount) {
		model_ = MyModel::Create(entityName + std::to_string(projectileCount), entityName, SceneType::Test);

		transform_.translate = { 0.0f, 0.0f, 0.0f };

		AABB hitbox;
		hitbox.min = { -0.5f, -0.5f, -0.5f };
		hitbox.max = { 0.5f, 0.5f, 0.5f };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(entityName + std::to_string(projectileCount), CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void Pistol::Update(float deltaTime) {
		transform_.translate.y += 5.0f * deltaTime;
	}

}