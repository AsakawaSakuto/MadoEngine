#include "EnemyManager.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Weapon/Projectile/ProjectileManager.h"
#include <algorithm>
#include <unordered_map>

namespace Enemy {

	void Manager::Initialize(Player::Base* player) {
		Clear();
		player_ = player;
		nextEnemyId_ = 0;
	}

	void Manager::Spawn(const SpawnDesc& desc) {
		std::unique_ptr<Base> enemy = std::make_unique<Base>();
		enemy->Initialize(nextEnemyId_++, desc);
		enemy->SetTargetPlayer(player_);
		enemies_.push_back(std::move(enemy));
	}

	void Manager::Update(float deltaTime) {
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (enemy) {
				enemy->Update(deltaTime);
			}
		}
	}

	void Manager::ResolveAfterCollision() {
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (enemy) {
				enemy->ResolveAfterCollision();
			}
		}

		ProcessProjectileHits();
		ProcessPlayerCollisions();
		RemoveInactiveEnemies();
	}

	void Manager::DrawDebugLine() const {
		for (const std::unique_ptr<Base>& enemy : enemies_) {
			if (enemy) {
				enemy->DrawDebugLine();
			}
		}
	}

	void Manager::Clear() {
		enemies_.clear();
		nextEnemyId_ = 0;
	}

	bool Manager::TryGetNearestEnemyPosition(Vector3& outPosition) const {
		if (!player_) {
			return false;
		}

		const Vector3 playerPosition = player_->GetPosition();
		float nearestDistanceSq = 0.0f;
		bool foundEnemy = false;

		for (const std::unique_ptr<Base>& enemy : enemies_) {
			if (!enemy || !enemy->IsActive()) {
				continue;
			}

			const Vector3 enemyPosition = enemy->GetPosition();
			const float distanceSq = (enemyPosition - playerPosition).LengthSq();
			if (!foundEnemy || distanceSq < nearestDistanceSq) {
				nearestDistanceSq = distanceSq;
				outPosition = enemyPosition;
				foundEnemy = true;
			}
		}

		return foundEnemy;
	}

	Vector3 Manager::GetNearestEnemyPosition() const {
		Vector3 nearestEnemyPosition = { 0.0f, 0.0f, 0.0f };
		TryGetNearestEnemyPosition(nearestEnemyPosition);
		return nearestEnemyPosition;
	}

	void Manager::ProcessProjectileHits() {
		std::vector<Projectile::EnemyTargetInfo> enemyTargets;
		enemyTargets.reserve(enemies_.size());

		std::unordered_map<std::uint32_t, Base*> enemiesById;
		enemiesById.reserve(enemies_.size());
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (!enemy || !enemy->IsActive()) {
				continue;
			}

			enemiesById.emplace(enemy->GetEnemyId(), enemy.get());
			enemyTargets.push_back({ enemy->GetEnemyId(), enemy->GetHitColliderName(), enemy->GetPosition() });
		}

		std::vector<Projectile::HitInfo> projectileHitInfos;
		Projectile::Manager::GetInstance().CollectHitsAgainst(enemyTargets, projectileHitInfos);
		for (const Projectile::HitInfo& hitInfo : projectileHitInfos) {
			const auto enemyIterator = enemiesById.find(hitInfo.enemyId);
			if (enemyIterator != enemiesById.end()) {
				enemyIterator->second->TakeProjectileDamage(hitInfo.projectileId, hitInfo.damage);
			}
		}
	}

	void Manager::ProcessPlayerCollisions() {
		if (!player_) {
			return;
		}

		const bool killAllEnemies = MyInput::GetKeybord()->IsTrigger(DIK_7);
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (!enemy || !enemy->IsActive()) {
				continue;
			}

			if (enemy->IsHitPlayer()) {
				player_->TakeDamage(enemy->GetPower());
				enemy->Kill();
				continue;
			}

			if (killAllEnemies) {
				enemy->Kill();
			}
		}
	}

	void Manager::RemoveInactiveEnemies() {
		enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
									  [](const std::unique_ptr<Base>& enemy) { return !enemy || !enemy->IsActive(); }),
					   enemies_.end());
	}

} // namespace Enemy
