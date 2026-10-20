#include "EnemyManager.h"
#include "Audio/MyAudio.h"
#include "EnemyFactory.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Weapon/Projectile/ProjectileManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Random.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace Enemy {
	namespace {
		constexpr float kGuaranteedStatusEffectChance = 100.0f;
		constexpr float kAttackTargetCellSize = 16.0f;

		/// @brief ワールド座標をXZ平面上の攻撃対象区画へ変換
		/// @param position 変換対象のワールド座標
		/// @param outCellX X方向の区画座標の格納先
		/// @param outCellZ Z方向の区画座標の格納先
		/// @return 区画座標へ変換できた場合はtrue
		bool TryGetAttackTargetCellCoordinates(const Vector3& position, int& outCellX, int& outCellZ) {
			const double cellX = std::floor(static_cast<double>(position.x) / kAttackTargetCellSize);
			const double cellZ = std::floor(static_cast<double>(position.z) / kAttackTargetCellSize);

			// 非有限値や整数範囲外の座標を索引化せず、変換時の未定義動作を回避
			if (!std::isfinite(cellX) || !std::isfinite(cellZ) ||
				cellX < std::numeric_limits<int>::min() || cellX > std::numeric_limits<int>::max() ||
				cellZ < std::numeric_limits<int>::min() || cellZ > std::numeric_limits<int>::max()) {
				return false;
			}

			outCellX = static_cast<int>(cellX);
			outCellZ = static_cast<int>(cellZ);
			return true;
		}

		/// @brief 符号付きのXZ区画座標を一意な索引キーへ変換
		/// @param cellX X方向の区画座標
		/// @param cellZ Z方向の区画座標
		/// @return 両座標の32ビット表現を結合した区画キー
		std::uint64_t CreateAttackTargetCellKey(int cellX, int cellZ) {
			return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cellX)) << 32) |
				static_cast<std::uint32_t>(cellZ);
		}

		/// @brief 状態異常種別をダメージ発生源種別へ変換
		/// @param type 変換する状態異常種別
		/// @return 対応するダメージ発生源種別
		Combat::DamageSourceType ToDamageSourceType(StatusEffect::Type type) {
			switch (type) {
			case StatusEffect::Type::Burn:
				return Combat::DamageSourceType::Burn;
			case StatusEffect::Type::Poison:
				return Combat::DamageSourceType::Poison;
			case StatusEffect::Type::Frozen:
			case StatusEffect::Type::Count:
			default:
				return Combat::DamageSourceType::Weapon;
			}
		}

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
		SynchronizeAttackTargetCell(*enemy);
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
					damageEvents_.push_back({
						enemy->GetPosition(),
						event.sourceWeaponId,
						event.appliedDamage,
						event.resolvedDamage,
						ToDamageSourceType(event.type),
						false,
						event.wasKilled,
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

				// 地形押し戻し後の確定座標を既存の更新走査中に索引へ反映
				SynchronizeAttackTargetCell(*enemy);
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

		// Enemyの所有権を解放する前に索引内の非所有参照を破棄
		attackTargetCells_.clear();
		attackTargetCellKeys_.clear();
		maxAttackTargetHorizontalExtent_ = 0.0f;
		enemies_.clear();
		damageEvents_.clear();
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

	void Manager::CollectNearestAttackTargetPositions(int maxCount, std::vector<Vector3>& outPositions) const {
		outPositions.clear();

		// Player未設定や空の索引では前回の候補を残さず探索を抑止
		if (!player_ || maxCount <= 0 || attackTargetCells_.empty()) {
			return;
		}

		const Vector3 playerPosition = player_->GetPosition();
		const Sphere attackRange = player_->GetAttackRangeSphere();
		const float queryRadius = attackRange.radius + maxAttackTargetHorizontalExtent_;
		int minCellX = 0;
		int minCellZ = 0;
		int maxCellX = 0;
		int maxCellZ = 0;

		// 中心が射程外でも大きいEnemyのAABBが射程に触れる区画まで検索範囲を拡張
		if (!std::isfinite(queryRadius) || queryRadius < 0.0f ||
			!TryGetAttackTargetCellCoordinates(attackRange.center - Vector3{ queryRadius, 0.0f, queryRadius }, minCellX, minCellZ) ||
			!TryGetAttackTargetCellCoordinates(attackRange.center + Vector3{ queryRadius, 0.0f, queryRadius }, maxCellX, maxCellZ)) {
			return;
		}

		const std::size_t candidateCount = std::min(static_cast<std::size_t>(maxCount), enemies_.size());
		outPositions.reserve(candidateCount);
		for (std::int64_t cellZ = minCellZ; cellZ <= maxCellZ; ++cellZ) {
			for (std::int64_t cellX = minCellX; cellX <= maxCellX; ++cellX) {

				// Player周辺の区画だけを参照し、遠方Enemyへの距離計算と衝突問い合わせを省略
				const std::uint64_t cellKey = CreateAttackTargetCellKey(static_cast<int>(cellX), static_cast<int>(cellZ));
				const auto cell = attackTargetCells_.find(cellKey);
				if (cell == attackTargetCells_.end()) {
					continue;
				}

				for (const Base* enemy : cell->second) {

					// 区画はXZ平面で絞り込み、高低差を含む実際の射程は球とAABBで最終判定
					if (!enemy->IsActive() || enemy->IsEmerging() ||
						!Collision::IsHit(attackRange, enemy->GetHitAABB())) {
						continue;
					}

					const Vector3 enemyPosition = enemy->GetPosition();
					const float distanceSq = (enemyPosition - playerPosition).LengthSq();
					if (!std::isfinite(distanceSq)) {
						continue;
					}

					// 周辺Enemyから距離順の上位件数だけを保持し、従来の抽選候補を維持
					const auto insertionPosition = std::upper_bound(outPositions.begin(), outPositions.end(), distanceSq,
						[&playerPosition](float candidateDistanceSq, const Vector3& position) {
							return candidateDistanceSq < (position - playerPosition).LengthSq();
						});
					if (outPositions.size() == candidateCount && insertionPosition == outPositions.end()) {
						continue;
					}

					outPositions.insert(insertionPosition, enemyPosition);
					if (outPositions.size() > candidateCount) {
						outPositions.pop_back();
					}
				}
			}
		}
	}

	void Manager::SynchronizeAttackTargetCell(Base& enemy) {
		int cellX = 0;
		int cellZ = 0;

		// 出現完了前や無効化済みのEnemyを索引に残さず、無効な座標の登録も抑止
		if (!enemy.IsActive() || enemy.IsEmerging() ||
			!TryGetAttackTargetCellCoordinates(enemy.GetPosition(), cellX, cellZ)) {
			RemoveAttackTargetCell(enemy);
			return;
		}

		const AABB hitBounds = enemy.GetHitAABB();
		const float horizontalExtent = std::max({
			std::abs(hitBounds.min.x), std::abs(hitBounds.max.x),
			std::abs(hitBounds.min.z), std::abs(hitBounds.max.z) });

		// 最大幅はClearまで保持し、最大個体の削除ごとの全Enemy再走査を回避
		maxAttackTargetHorizontalExtent_ = std::max(maxAttackTargetHorizontalExtent_, horizontalExtent);
		const std::uint64_t cellKey = CreateAttackTargetCellKey(cellX, cellZ);
		const auto currentCell = attackTargetCellKeys_.find(enemy.GetEnemyId());
		if (currentCell != attackTargetCellKeys_.end() && currentCell->second == cellKey) {
			return;
		}

		// 区画を越えた場合だけ所属を差し替え、同一区画内の移動ではContainer変更を省略
		RemoveAttackTargetCell(enemy);
		attackTargetCells_[cellKey].push_back(&enemy);
		attackTargetCellKeys_.emplace(enemy.GetEnemyId(), cellKey);
	}

	void Manager::RemoveAttackTargetCell(const Base& enemy) {
		const auto currentCell = attackTargetCellKeys_.find(enemy.GetEnemyId());
		if (currentCell == attackTargetCellKeys_.end()) {
			return;
		}

		// Enemy破棄前に非所有参照を除外し、空区画も解放して遠方の履歴蓄積を防止
		const auto cell = attackTargetCells_.find(currentCell->second);
		if (cell != attackTargetCells_.end()) {
			std::erase(cell->second, &enemy);
			if (cell->second.empty()) {
				attackTargetCells_.erase(cell);
			}
		}
		attackTargetCellKeys_.erase(currentCell);
	}

	std::vector<Combat::DamageEvent> Manager::ConsumeDamageEvents() {
		std::vector<Combat::DamageEvent> events;

		// 未処理Eventの所有権を定数時間で呼び出し側へ移動
		events.swap(damageEvents_);
		return events;
	}

	void Manager::CollectStatusEffectDisplayData(
		std::vector<StatusEffectDisplayData>& outDisplayData) const {
		outDisplayData.clear();
		outDisplayData.reserve(enemies_.size());

		// 描画側へEnemy本体の参照を渡さず、表示に必要な現在値だけを複製
		for (const std::unique_ptr<Base>& enemy : enemies_) {
			if (!enemy || !enemy->IsActive() || enemy->IsEmerging()) {
				continue;
			}

			StatusEffectDisplayData displayData;
			displayData.enemyId = enemy->GetEnemyId();
			displayData.anchorPosition = enemy->GetStatusEffectAnchorPosition();
			for (std::size_t index = 0; index < StatusEffect::kTypeCount; ++index) {
				displayData.activeEffects[index] =
					enemy->HasStatusEffect(static_cast<StatusEffect::Type>(index));
			}
			outDisplayData.push_back(displayData);
		}
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
		damageEvents_.reserve(damageEvents_.size() + projectileHitInfos.size());
		for (const Projectile::HitInfo& hitInfo : projectileHitInfos) {
			const auto enemyIterator = enemiesById.find(hitInfo.enemyId);
			if (enemyIterator == enemiesById.end()) {
				continue;
			}

			Base* enemy = enemyIterator->second;
			const bool dealsRemainingHealthDamage =
				hitInfo.damageMode == Projectile::DamageMode::RemainingHealth &&
				enemy->GetType() != Data::Type::Boss;

			// 即死抽選成功時は適用直前の残りHPを使い、Critical倍率による過剰Damageを抑制
			const float damage = [&]() {
				if (dealsRemainingHealthDamage) {
					return (enemy->GetCurrentHealth());
				} else {
					return (hitInfo.damage);
				}
			}();
			const float criticalChance = [&]() {
				if (dealsRemainingHealthDamage) {
					return (0.0f);
				} else {
					return (hitInfo.criticalChance);
				}
			}();
			const float criticalDamage = [&]() {
				if (dealsRemainingHealthDamage) {
					return (1.0f);
				} else {
					return (hitInfo.criticalDamage);
				}
			}();
			const DamageResult damageResult =
				enemy->TakeProjectileDamage(
					hitInfo.projectileId,
					damage,
					criticalChance,
					criticalDamage,
					hitInfo.knockbackDirection,
					hitInfo.knockbackPower);
			if (!damageResult.wasApplied) {
				continue;
			}
			const bool wasInstantKilled = dealsRemainingHealthDamage && damageResult.wasKilled;

			// Trumpの即死成立時だけ専用SEを重ねて通常撃破と区別
			if (wasInstantKilled && MyAudio::IsLoaded("Joker")) {
				MyAudio::Play("Joker");
			}

			damageEvents_.push_back({
				enemy->GetPosition(),
				hitInfo.sourceWeaponId,
				damageResult.appliedDamage,
				damageResult.resolvedDamage,
				Combat::DamageSourceType::Weapon,
				damageResult.isCritical,
				damageResult.wasKilled,
				[&]() {
					if (wasInstantKilled) {
						return (Combat::DamageDisplayType::InstantKill);
					} else {
						return (Combat::DamageDisplayType::Number);
					}
				}(),
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

		const bool killAllEnemies = MyInput::GetKeyboard()->IsTrigger(DIK_7);
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
			[this](const std::unique_ptr<Base>& enemy) {
				if (!enemy) {
					return true;
				}
				if (enemy->IsActive()) {
					return false;
				}

				// 所有Containerの圧縮でEnemyが破棄される前に区画側の参照を解除
				RemoveAttackTargetCell(*enemy);
				return true;
			}),
			enemies_.end());
	}

} // namespace Enemy
