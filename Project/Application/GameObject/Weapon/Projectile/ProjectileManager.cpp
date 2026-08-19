#include "ProjectileManager.h"
#include <algorithm>
#include <utility>

namespace Projectile {
	namespace {
		constexpr float kMinBounceDirectionLengthSq = 0.000001f;

		/// @brief 衝突したEnemy以外で最も近い跳弾先を検索
		/// @param projectile 跳弾するProjectile
		/// @param collidedEnemyId 衝突したEnemyの識別番号
		/// @param enemyTargets 跳弾先候補
		/// @return 跳弾先が存在する場合はその情報を、存在しない場合はnullptr
		const EnemyTargetInfo* FindNearestBounceTarget(
			const IProjectile& projectile,
			std::uint32_t collidedEnemyId,
			const std::vector<EnemyTargetInfo>& enemyTargets) {
			const EnemyTargetInfo* nearestTarget = nullptr;
			float nearestDistanceSq = 0.0f;

			// 現在の衝突相手と方向を作れない同位置の候補を除外して距離比較
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

		// Update中に派生Projectileが追加されてもContainerを無効化しない走査状態
		isTraversingProjectiles_ = true;
		for (auto& projectile : projectiles) {
			if (projectile && !projectile->IsDead()) {
				projectile->Update(deltaTime);
			}
		}
		isTraversingProjectiles_ = false;

		// 全Projectileの更新完了後に寿命切れを一括破棄
		projectiles.erase(
			std::remove_if(projectiles.begin(), projectiles.end(), [](const std::unique_ptr<IProjectile>& projectile) {
				return projectile->IsDead();
			}),
			projectiles.end()
		);

		FlushPendingProjectiles();
	}

	void Manager::AddProjectile(Projectile::Type type, InitializeDesc context) {
		if (isTraversingProjectiles_) {

			// 走査中の再配置を避けるため生成要求だけを退避
			pendingProjectileAddRequests_.push_back({ type, std::move(context) });
			return;
		}

		AddProjectileImmediate(type, std::move(context));
	}

	bool Manager::SynchronizePersistentProjectile(Projectile::Type type, InitializeDesc context) {
		if (!IsPersistentWeaponType(type) || context.sourceWeaponId == 0) {
			return false;
		}

		// 発射元Weaponごとに常時展開Projectileを一個へ限定
		for (const std::unique_ptr<IProjectile>& projectile : projectiles) {
			if (!projectile || projectile->IsDead() ||
				projectile->GetSourceWeaponId() != context.sourceWeaponId) {
				continue;
			}

			projectile->SynchronizePersistentState(context);
			return false;
		}

		AddProjectile(type, std::move(context));
		return true;
	}

	void Manager::RemoveProjectilesBySourceWeaponId(std::uint64_t sourceWeaponId) {
		if (sourceWeaponId == 0) {
			return;
		}

		// 管理列の走査状態に依存せず次回Updateの一括破棄へ集約
		for (const std::unique_ptr<IProjectile>& projectile : projectiles) {
			if (projectile && projectile->GetSourceWeaponId() == sourceWeaponId) {
				projectile->RequestDestroy();
			}
		}
	}

	void Manager::Clear() {
		pendingProjectileAddRequests_.clear();
		projectiles.clear();
		nextProjectileId_ = 1;
		isTraversingProjectiles_ = false;
	}

	void Manager::AddProjectileImmediate(Projectile::Type type, InitializeDesc context) {

		// 衝突結果を後から照合できるよう全種類で一意なIDを採番
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
		case Projectile::Type::Bow: {
			auto bow = std::make_unique<Bow>();
			bow->Initialize(context);
			projectiles.push_back(std::move(bow));
			break;
		}
		case Projectile::Type::Eye: {
			auto eye = std::make_unique<Eye>();
			eye->Initialize(context);
			projectiles.push_back(std::move(eye));
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

	void Manager::FlushPendingProjectiles() {
		if (pendingProjectileAddRequests_.empty()) {
			return;
		}

		std::vector<ProjectileAddRequest> requests = std::move(pendingProjectileAddRequests_);
		pendingProjectileAddRequests_.clear();

		// 元Bufferを先に空にして追加処理からの新規要求と分離
		for (ProjectileAddRequest& request : requests) {
			AddProjectileImmediate(request.type, std::move(request.context));
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
		isTraversingProjectiles_ = true;

		// Projectile単位で当Frameの接触履歴を管理して同一Enemyへの重複Hitを抑制
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

					// 現在の衝突相手を除いた最近傍Enemyへ跳弾方向を更新
					bounceTarget = FindNearestBounceTarget(*projectile, enemyTarget.enemyId, enemyTargets);
				}

				const Vector3* bounceTargetPosition = bounceTarget ? &bounceTarget->position : nullptr;
				if (!projectile->HandleEnemyCollision(enemyTarget.enemyId, bounceTargetPosition)) {
					continue;
				}

				// 貫通数や跳弾状態の確定後にProjectile固有のHit効果を適用
				projectile->OnEnemyHit();

				outHitInfos.push_back({
					enemyTarget.enemyId,
					projectile->GetProjectileId(),
					projectile->GetSourceWeaponId(),
					projectile->GetDamage()
				});

				if (projectile->IsDead()) {
					break;
				}
			}

			projectile->EndEnemyCollisionFrame();
		}
		isTraversingProjectiles_ = false;

		// Hit時に派生生成されたExplosionなどを衝突走査完了後に反映
		FlushPendingProjectiles();
	}
}
