#include "EnemyManager.h"
#include "EnemyFactory.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Weapon/Projectile/ProjectileManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Random.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Enemy {
	namespace {
		constexpr float kGuaranteedStatusEffectChance = 100.0f;

		/// @brief 状態異常の付与率から発症可否を抽選
		/// @param applyChance 状態異常の付与率
		/// @return 状態異常を付与する場合はtrue
		bool RollStatusEffectApplication(float applyChance) {
			if (!std::isfinite(applyChance) || applyChance <= 0.0f) {
				return false;
			}

			return applyChance >= kGuaranteedStatusEffectChance ||
				MyRand::GetFloat(0.0f, kGuaranteedStatusEffectChance) < applyChance;
		}

		/// @brief 継続ダメージ型状態異常を抽選してEnemyへ適用
		/// @param enemy 適用対象のEnemy
		/// @param type 適用する状態異常の種類
		/// @param status 武器に設定された状態異常性能
		/// @param sourceWeaponId 適用元の武器識別番号
		void TryApplyDamageStatusEffect(
			Base& enemy,
			StatusEffect::Type type,
			const Weapon::DamageStatusEffectStatus& status,
			std::uint64_t sourceWeaponId) {
			if (!RollStatusEffectApplication(status.applyChance)) {
				return;
			}

			StatusEffect::ApplyRequest request;
			request.type = type;
			request.definition.duration = status.duration;
			request.definition.damagePerTick = status.damagePerTick;
			request.sourceWeaponId = sourceWeaponId;
			enemy.ApplyStatusEffect(request);
		}

		/// @brief 凍結状態異常を抽選してEnemyへ適用
		/// @param enemy 適用対象のEnemy
		/// @param status 武器に設定された状態異常性能
		/// @param sourceWeaponId 適用元の武器識別番号
		void TryApplyFrozenStatusEffect(
			Base& enemy,
			const Weapon::FrozenStatusEffectStatus& status,
			std::uint64_t sourceWeaponId) {
			if (!RollStatusEffectApplication(status.applyChance)) {
				return;
			}

			StatusEffect::ApplyRequest request;
			request.type = StatusEffect::Type::Frozen;
			request.definition.duration = status.duration;
			request.definition.moveSpeedMultiplier = status.moveSpeedMultiplier;
			request.sourceWeaponId = sourceWeaponId;
			enemy.ApplyStatusEffect(request);
		}

		/// @brief 武器に設定された状態異常を個別に抽選してEnemyへ適用
		/// @param enemy 適用対象のEnemy
		/// @param statusEffects 武器に設定された状態異常群
		/// @param sourceWeaponId 適用元の武器識別番号
		void TryApplyStatusEffects(
			Base& enemy,
			const Weapon::StatusEffectStatus& statusEffects,
			std::uint64_t sourceWeaponId) {

			// 複数種類を設定した武器では各状態異常を独立抽選して同時発症を許可
			if (statusEffects.burn) {
				TryApplyDamageStatusEffect(
					enemy, StatusEffect::Type::Burn, *statusEffects.burn, sourceWeaponId);
			}
			if (statusEffects.poison) {
				TryApplyDamageStatusEffect(
					enemy, StatusEffect::Type::Poison, *statusEffects.poison, sourceWeaponId);
			}
			if (statusEffects.frozen) {
				TryApplyFrozenStatusEffect(enemy, *statusEffects.frozen, sourceWeaponId);
			}
		}
	}

	void Manager::Initialize(Player::Base* player) {
		Clear();
		player_ = player;
		nextEnemyId_ = 0;
	}

	void Manager::Spawn(const SpawnDesc& desc) {
		std::unique_ptr<Base> enemy = Factory::Create(desc.type);
		if (!enemy) {
			Logger::Output("未対応の種類が指定されたためEnemyの生成に失敗しました", Logger::Level::Warning);
			return;
		}

		SpawnDesc effectiveDesc = desc;
		if (effectiveDesc.type == Data::Type::Boss) {

			// Bossへ外部からEliteが指定されても属性制約を維持
			effectiveDesc.bonusType = Data::BonusType::None;
		}

		enemy->Initialize(nextEnemyId_++, effectiveDesc);
		enemy->SetTargetPlayer(player_);
		enemy->SetMapLimit(mapLimit_);
		enemies_.push_back(std::move(enemy));
	}

	void Manager::SpawnBoss(const Vector3& position, SceneType sceneType) {
		SpawnDesc desc;
		desc.position = position;
		desc.status = Factory::CreateDefaultStatus(Data::Type::Boss);
		desc.type = Data::Type::Boss;
		desc.bonusType = Data::BonusType::None;
		desc.sceneType = sceneType;
		Spawn(desc);

		Logger::Output("Bossを生成しました", Logger::Level::Application);
	}

	void Manager::Update(float deltaTime) {
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (enemy) {
				enemy->Update(deltaTime);
				for (const StatusEffectDamageEvent& event : enemy->ConsumeStatusEffectDamageEvents()) {

					// 継続Damageも表示と武器戦績へ通知できる共通Event形式へ変換
					projectileDamageEvents_.push_back({
						enemy->GetPosition(),
						event.sourceWeaponId,
						event.appliedDamage,
						event.resolvedDamage,
						false,
						event.wasKilled,
						event.type,
					});
				}
			}
		}
	}

	void Manager::SetMapLimit(const MapLimit& mapLimit) {
		mapLimit_ = mapLimit;

		// 再生成前から存在するEnemyにも同じ外周制限を反映
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (enemy) {
				enemy->SetMapLimit(mapLimit_);
			}
		}
	}

	void Manager::ResolveAfterCollision() {

		// 全EnemyへCollider解決結果を反映してから相互作用と削除をまとめて処理
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (enemy) {
				enemy->ResolveAfterCollision();
			}
		}

		DeactivateEnemiesOutsidePlayerRange();
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
		projectileDamageEvents_.clear();
		nextEnemyId_ = 0;
	}

	bool Manager::TryGetNearestEnemyPosition(Vector3& outPosition) const {
		if (!player_) {
			return false;
		}

		const Vector3 playerPosition = player_->GetPosition();
		float nearestDistanceSq = 0.0f;
		bool foundEnemy = false;

		// 平方根を避けた距離比較で有効なEnemyだけを探索
		for (const std::unique_ptr<Base>& enemy : enemies_) {
			if (!enemy || !enemy->IsActive() || enemy->IsEmerging()) {
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

	std::vector<ProjectileDamageEvent> Manager::ConsumeProjectileDamageEvents() {
		std::vector<ProjectileDamageEvent> events;

		// 未処理Eventの所有権を定数時間で呼び出し側へ移動
		events.swap(projectileDamageEvents_);
		return events;
	}

	void Manager::ProcessProjectileHits() {
		std::vector<Projectile::EnemyTargetInfo> enemyTargets;
		enemyTargets.reserve(enemies_.size());

		std::unordered_map<std::uint32_t, Base*> enemiesById;
		enemiesById.reserve(enemies_.size());

		// Projectile側へ渡すSnapshotとHit結果を戻すID索引を同時に構築
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (!enemy || !enemy->IsActive() || enemy->IsEmerging()) {
				continue;
			}

			enemiesById.emplace(enemy->GetEnemyId(), enemy.get());
			enemyTargets.push_back({ enemy->GetEnemyId(), enemy->GetHitColliderName(), enemy->GetPosition() });
		}

		std::vector<Projectile::HitInfo> projectileHitInfos;
		Projectile::Manager::GetInstance().CollectHitsAgainst(enemyTargets, projectileHitInfos);
		projectileDamageEvents_.reserve(projectileDamageEvents_.size() + projectileHitInfos.size());
		for (const Projectile::HitInfo& hitInfo : projectileHitInfos) {
			const auto enemyIterator = enemiesById.find(hitInfo.enemyId);
			if (enemyIterator == enemiesById.end()) {
				continue;
			}

			Base* enemy = enemyIterator->second;
			const DamageResult damageResult =
				enemy->TakeProjectileDamage(
					hitInfo.projectileId,
					hitInfo.damage,
					hitInfo.criticalChance,
					hitInfo.criticalDamage,
					hitInfo.knockbackDirection,
					hitInfo.knockbackPower);
			if (!damageResult.wasApplied) {
				continue;
			}

			projectileDamageEvents_.push_back({
				enemy->GetPosition(),
				hitInfo.sourceWeaponId,
				damageResult.appliedDamage,
				damageResult.resolvedDamage,
				damageResult.isCritical,
				damageResult.wasKilled,
				std::nullopt,
			});

			// 撃破済みEnemyへの不要な状態登録を避け、実ダメージ成立後だけ付与抽選
			if (!damageResult.wasKilled) {
				TryApplyStatusEffects(*enemy, hitInfo.statusEffects, hitInfo.sourceWeaponId);
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

			enemy->ResolvePlayerCollision(*player_);

			if (killAllEnemies) {
				enemy->Kill();
			}
		}
	}

	void Manager::DeactivateEnemiesOutsidePlayerRange() {

		// 範囲外Enemyを相互作用処理より先に無効化して同Frameの撃破報酬生成を防止
		for (std::unique_ptr<Base>& enemy : enemies_) {
			if (!enemy || !enemy->IsActive()) {
				continue;
			}

			// BossだけをPlayer周辺の管理範囲外でも維持
			const Data::Type type = enemy->GetType();
			if (type == Data::Type::Boss || enemy->IsInsidePlayerDeleteRange()) {
				continue;
			}

			enemy->Kill(DeathReason::OutsidePlayerRange);
		}
	}

	void Manager::RemoveInactiveEnemies() {

		// 判定中のContainer無効化を避けるため全相互作用の完了後にまとめて削除
		enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
									  [](const std::unique_ptr<Base>& enemy) { return !enemy || !enemy->IsActive(); }),
					   enemies_.end());
	}

} // namespace Enemy
