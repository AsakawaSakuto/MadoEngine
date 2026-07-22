#pragma once
#include "../Engine/UtilityHeaders.h"

/// @brief ゲーム内の衝突判定の組み合わせを登録する関数
void RegisterColliderPair() {

	MyCollider::RegisterCollisionPair(CollisionTag::MapLimitBox, CollisionTag::PlayerProjectileHitBox, false);

	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::MapSlope, true);
	MyCollider::RegisterCollisionPair(CollisionTag::EnemyMovementSphere, CollisionTag::EnemyMovementSphere, true);

	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock, true);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, true);

	MyCollider::RegisterCollisionPair(CollisionTag::EnemyHitBox, CollisionTag::PlayerProjectileHitBox, false);
	MyCollider::RegisterCollisionPair(CollisionTag::EnemyHitBox, CollisionTag::PlayerHitBox, false);

	MyCollider::RegisterCollisionPair(CollisionTag::PlayerDropObjectGetSphere, CollisionTag::DropObjectHitBox, false);
	MyCollider::RegisterCollisionPair(CollisionTag::PlayerHitBox, CollisionTag::DropObjectHitBox, false);

}