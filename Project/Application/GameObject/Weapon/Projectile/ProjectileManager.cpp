#include "ProjectileManager.h"
#include <algorithm>

namespace Projectile {
	namespace {
		constexpr float kMinBounceDirectionLengthSq = 0.000001f;

		/// @brief 衝突したEnemy以外で最も近い跳弾先を検索
		/// @param projectile 跳弾するProjectile
		/// @param collidedEnemyId 衝突したEnemyの識別番号
		/// @param enemyTargets 跳弾先候補
		/// @return 跳弾先が存在する場合はその情報を、存在しない場合はnullptrを返す
		const EnemyTargetInfo* FindNearestBounceTarget(
			const IProjectile& projectile,
			std::uint32_t collidedEnemyId,
			const std::vector<EnemyTargetInfo>& enemyTargets) {
			const EnemyTargetInfo* nearestTarget = nullptr;
			float nearestDistanceSq = 0.0f;

			for (const EnemyTargetInfo& candidate : enemyTargets) {
				if (candidate.enemyId == collidedEnemyId || candidate.colliderName.empty()) {
					continue;
				}

				const float distanceSq = (candidate.position - projectile.GetPosition()).LengthSq();
				if (distanceSq <= kMinBounceDirectionLengthSq) {
					continue;
				}

				if (!nearestTarget || distanceSq < nearestDistanceSq) {
					nearestTarget = &candidate;
					nearestDistanceSq = distanceSq;
				}
			}

			return nearestTarget;
		}
	}

	Manager& Manager::GetInstance() {
		static Manager instance;
		return instance;
	}

	void Manager::Update(float deltaTime) {
		for (auto& projectile : projectiles) {
			if (projectile && !projectile->IsDead()) {
				projectile->Update(deltaTime);
			}
		}

		projectiles.erase(
			std::remove_if(projectiles.begin(), projectiles.end(), [](const std::unique_ptr<IProjectile>& projectile) {
				return projectile->IsDead();
			}),
			projectiles.end()
		);
	}

	void Manager::AddProjectile(Projectile::Type type, InitializeDesc context) {
		context.projectileId = nextProjectileId_++;

		switch (type) {
		case Projectile::Type::Explosion: {
			auto explosion = std::make_unique<Explosion>();
			explosion->Initialize(context);
			projectiles.push_back(std::move(explosion));
			break;
		}
		case Projectile::Type::Pistol: {
			auto pistol = std::make_unique<Pistol>();
			pistol->Initialize(context);
			projectiles.push_back(std::move(pistol));
			break;
		}
		case Projectile::Type::Rock: {
			auto rock = std::make_unique<Rock>();
			rock->Initialize(context);
			projectiles.push_back(std::move(rock));
			break;
		}
		case Projectile::Type::FireBall: {
			auto fireBall = std::make_unique<FireBall>();
			fireBall->Initialize(context);
			projectiles.push_back(std::move(fireBall));
			break;
		}
		case Projectile::Type::Axe: {
			auto axe = std::make_unique<Axe>();
			axe->Initialize(context);
			projectiles.push_back(std::move(axe));
			break;
		}
		default:
			break;
		}
	}

	void Manager::CollectHitsAgainst(
		const std::vector<EnemyTargetInfo>& enemyTargets,
		std::vector<HitInfo>& outHitInfos) {
		outHitInfos.clear();
		if (enemyTargets.empty()) {
			return;
		}

		outHitInfos.reserve(projectiles.size());
		for (const std::unique_ptr<IProjectile>& projectile : projectiles) {
			if (!projectile || projectile->IsDead() || projectile->GetColliderName().empty()) {
				continue;
			}

			projectile->BeginEnemyCollisionFrame();
			for (const EnemyTargetInfo& enemyTarget : enemyTargets) {
				if (enemyTarget.colliderName.empty() ||
					!MyCollider::IsHitName(enemyTarget.colliderName, projectile->GetColliderName())) {
					continue;
				}

				const EnemyTargetInfo* bounceTarget = nullptr;
				if (projectile->CanBounce()) {
					bounceTarget = FindNearestBounceTarget(*projectile, enemyTarget.enemyId, enemyTargets);
				}

				const Vector3* bounceTargetPosition = bounceTarget ? &bounceTarget->position : nullptr;
				if (!projectile->HandleEnemyCollision(enemyTarget.enemyId, bounceTargetPosition)) {
					continue;
				}

				outHitInfos.push_back({
					enemyTarget.enemyId,
					projectile->GetProjectileId(),
					projectile->GetDamage()
				});

				if (projectile->IsDead()) {
					break;
				}
			}

			projectile->EndEnemyCollisionFrame();
		}
	}
}
