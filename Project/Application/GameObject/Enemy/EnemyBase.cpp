#include "EnemyBase.h"
#include "EnemySettings.h"
#include "EnemyStatusEffectVisualSettings.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "GameObject/Player/Player.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Random.h"
#include <algorithm>
#include <cmath>

namespace Enemy {
	namespace {
		constexpr float kDamageFlashDuration = 6.0f / 60.0f;
		constexpr float kEmergenceSpeed = 4.0f;
		constexpr float kEmergenceCompletionEpsilon = 1e-4f;
		constexpr float kPlayerKnockbackPower = 1.5f;
		constexpr float kPlayerKnockbackDirectionEpsilonSq = 0.000001f;
		constexpr int kCriticalRollMin = 1;
		constexpr int kCriticalRollMax = 100;
		constexpr float kGuaranteedCriticalChance = static_cast<float>(kCriticalRollMax);
		constexpr Vector4 kDamageFlashColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr Vector4 kEliteMarkerColor = { 1.0f, 1.0f, 1.0f, 0.999f };
		constexpr const char* kEliteMarkerModelAssetName = "Plane";          
		constexpr const char* kEliteMarkerTextureName = "Elite";

		/// @brief Enemy基礎色へ有効な状態異常の色係数を合成
		/// @param baseColor Enemy種類ごとの基礎色
		/// @param controller 状態異常の管理Controller
		/// @return 状態異常色を乗算した表示色
		Vector4 ApplyStatusEffectColorMultipliers(
			const Vector4& baseColor,
			const StatusEffect::Controller& controller) {
			Vector4 result = baseColor;
			for (std::size_t index = 0; index < StatusEffect::kTypeCount; ++index) {
				const StatusEffect::Type type = static_cast<StatusEffect::Type>(index);
				if (!controller.IsActive(type)) {
					continue;
				}

				// 複数状態を同時に識別できるよう単一色への置換ではなく係数を順番に合成
				result *= StatusEffect::VisualSettings::GetInstance().Get(type).enemyColorMultiplier;
			}

			// 高い色係数による表示範囲超過を防ぎAlphaはEnemy基礎色を維持
			result.x = std::clamp(result.x, 0.0f, 1.0f);
			result.y = std::clamp(result.y, 0.0f, 1.0f);
			result.z = std::clamp(result.z, 0.0f, 1.0f);
			result.w = std::clamp(baseColor.w, 0.0f, 1.0f);
			return result;
		}
	}

	Base::~Base() { Release(); }

	void Base::Initialize(std::uint32_t enemyId, const SpawnDesc& desc) {
		enemyId_ = enemyId;
		status_ = desc.status;
		type_ = desc.type;
		bonusType_ = desc.bonusType;
		sceneType_ = desc.sceneType;
		const TypeSettings& typeSettings = Settings::GetInstance().GetTypeSettings(type_);
		const EliteSettings& eliteSettings = Settings::GetInstance().GetEliteSettings();
		bodyScaleMultiplier_ = bonusType_ == Data::BonusType::Elite ? eliteSettings.bodyScaleMultiplier : 1.0f;
		knockbackResistance_ = std::clamp(typeSettings.knockbackResistance, 0.0f, 1.0f);
		if (bonusType_ == Data::BonusType::Elite) {

			// 時間経過補正後の基礎能力値へElite倍率を重ねて全非Boss種類へ同じ属性効果を適用
			status_.currentHealth *= eliteSettings.healthMultiplier;
			status_.power *= eliteSettings.powerMultiplier;
		}
		projectileDamageCooldowns_.clear();
		statusEffectDamageEvents_.clear();
		playerDamageCooldown_ = 0.0f;
		damageFlashRemainingTime_ = 0.0f;
		isActive_ = status_.currentHealth > 0.0f;
		isEmerging_ = false;
		areCollidersRegistered_ = false;
		isDeathRewardSpawned_ = false;
		isReleased_ = false;
		transform_.translate = desc.position;
		transform_.rotate = {};
		transform_.scale = GetModelScale() * bodyScaleMultiplier_;
		movement_.Initialize();
		statusEffectController_.Clear();

		AABB hitCollider = CreateHitCollider();
		hitCollider.min *= bodyScaleMultiplier_;
		hitCollider.max *= bodyScaleMultiplier_;
		hitAABB_ = hitCollider;
		Sphere movementCollider = CreateMovementCollider();
		movementCollider.radius *= bodyScaleMultiplier_;
		colliderShape_ = movementCollider;
		emergenceTargetY_ = desc.groundSurfaceY + movementCollider.radius;
		isEmerging_ = desc.emergeFromGround && std::isfinite(emergenceTargetY_) &&
			transform_.translate.y < emergenceTargetY_;

		movementColliderName_ = CreateColliderName("EnemyMovementSphere");
		hitColliderName_ = CreateColliderName("EnemyHitBox");
		modelName_ = CreateModelName();

		// 出現中の地形押し戻しと攻撃判定を避けるため地表面到達後までCollider登録を保留
		if (!isEmerging_) {
			RegisterColliders();
		}

		std::string modelAssetName = GetModelAssetName();
		if (!MadoEngine::ModelManager::GetInstance().GetSharedData(modelAssetName)) {

			// 専用Modelが未配置でもEnemyの当たり判定と行動を検証できるよう共通Modelへ代替
			Logger::Output(
				"Enemy用Modelアセットが見つからないためenemyへ切り替えます: " + modelAssetName,
				Logger::Level::Warning);
			modelAssetName = "enemy";
		}

		model_ = MyModel::Create(modelName_, modelAssetName, desc.sceneType);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetRenderLayer(MadoEngine::Render::RenderLayer::Enemy);
			model->SetTexture("white16x16");
			model->SetColor(Settings::GetInstance().GetTypeSettings(type_).color);
		}
		CreateEliteMarkerModel();

		ApplyModelTransform();
		OnInitialized();
	}

	void Base::Update(float deltaTime) {

		// 死亡後も残る被弾間隔を破棄まで進行
		UpdateProjectileDamageCooldowns(deltaTime);
		playerDamageCooldown_ = std::max(0.0f, playerDamageCooldown_ - std::max(0.0f, deltaTime));

		// 出現中は攻撃対象外のため状態異常を進行させず、通常状態へ移行後から更新
		if (isActive_ && !isEmerging_) {
			UpdateStatusEffects(deltaTime);
		}

		// 状態異常の発症と解除を同FrameのModel色へ反映して被弾Flashの時間も更新
		UpdateAppearance(deltaTime);
		if (!isActive_) {
			return;
		}

		if (!targetPlayer_) {
			return;
		}

		if (isEmerging_) {

			// 地中では追跡と重力を停止して地表面へ向かう出現移動だけを更新
			UpdateEmergence(deltaTime);
			return;
		}

		// 共通状態の検証後に種類固有の行動へ更新を委譲
		UpdateBehavior(deltaTime);
	}

	void Base::ResolveAfterCollision() {
		if (!isActive_) {
			return;
		}

		if (!isEmerging_) {
			movement_.ResolveAfterCollision(movementColliderName_, transform_);
		}
		ApplyModelTransform();
	}

	void Base::DrawDebugLine() const {
		if (!isActive_ || isEmerging_) {
			return;
		}

		const Vector4 movementColliderColor = { 0.0f, 1.0f, 0.0f, 1.0f };
		const Vector4 hitColliderColor = { 1.0f, 0.0f, 0.0f, 1.0f };
		MyDebugLine::AddShape(colliderShape_, movementColliderColor);
		MyDebugLine::AddShape(hitAABB_, hitColliderColor);
	}

	bool Base::IsHitPlayer() const {
		if (!isActive_ || isEmerging_) {
			return false;
		}

		return MyCollider::IsHitWithTag(hitColliderName_, CollisionTag::PlayerHitBox);
	}

	bool Base::IsInsidePlayerDeleteRange() const {
		if (!isActive_) {
			return false;
		}
		if (isEmerging_) {

			// Collider登録前の出現中Enemyを範囲外として誤削除しないため管理範囲内扱い
			return true;
		}

		return MyCollider::IsHitWithTag(hitColliderName_, CollisionTag::EnemyDeleteRangeSphere);
	}

	bool Base::ResolvePlayerCollision(Player::Base& player) {
		if (!IsHitPlayer() || playerDamageCooldown_ > 0.0f) {
			return false;
		}

		// Bossの継続接触を考慮して種類別の待機時間をDamage適用前に確定
		playerDamageCooldown_ = std::max(0.0f, GetPlayerDamageInterval());
		player.TakeDamage(status_.power);

		Vector3 knockbackDirection = player.GetPosition() - transform_.translate;
		knockbackDirection.y = 0.0f;
		if (knockbackDirection.LengthSq() <= kPlayerKnockbackDirectionEpsilonSq) {

			// 中心が一致した場合も押し出せるようEnemyの正面方向へ代替
			knockbackDirection = {
				std::sin(transform_.rotate.y),
				0.0f,
				std::cos(transform_.rotate.y),
			};
		}
		player.ApplyKnockback(knockbackDirection, kPlayerKnockbackPower);

		return true;
	}

	DamageResult Base::TakeProjectileDamage(
		std::uint64_t projectileId,
		float damage,
		float criticalChance,
		float criticalDamage,
		const Vector3& knockbackDirection,
		float knockbackPower) {
		DamageResult result;

		// 不正なProjectile識別子と非有限Damageを状態へ反映しないため入力を検証
		if (!isActive_ || isEmerging_ || projectileId == 0 || !std::isfinite(damage) || damage <= 0.0f) {
			return result;
		}

		if (projectileDamageCooldowns_.contains(projectileId)) {
			return result;
		}

		// 確定率以上では乱数消費を避け、それ未満では1から100の整数値で命中ごとに抽選
		const bool isCritical = criticalChance >= kGuaranteedCriticalChance ||
			static_cast<float>(MyRand::GetInt(kCriticalRollMin, kCriticalRollMax)) <= criticalChance;
		const float resolvedDamage = isCritical ? damage * criticalDamage : damage;
		if (!std::isfinite(resolvedDamage) || resolvedDamage <= 0.0f) {
			return result;
		}

		result = ApplyDamage(resolvedDamage, isCritical, true);
		if (result.wasApplied) {

			// 同じProjectileが接触中に毎フレームDamageを与えないよう適用成功後に待機時間を登録
			projectileDamageCooldowns_.emplace(projectileId, projectileDamageInterval_);
			const float effectiveKnockbackPower = knockbackPower * (1.0f - knockbackResistance_);
			movement_.ApplyKnockback(knockbackDirection, effectiveKnockbackPower);
		}

		return result;
	}

	bool Base::ApplyStatusEffect(const StatusEffect::ApplyRequest& request) {
		if (!isActive_ || isEmerging_) {
			return false;
		}

		return statusEffectController_.Apply(request);
	}

	bool Base::HasStatusEffect(StatusEffect::Type type) const {
		return statusEffectController_.IsActive(type);
	}

	float Base::GetStatusEffectRemainingTime(StatusEffect::Type type) const {
		return statusEffectController_.GetRemainingTime(type);
	}

	std::vector<StatusEffectDamageEvent> Base::ConsumeStatusEffectDamageEvents() {
		std::vector<StatusEffectDamageEvent> events;

		// 未処理Eventの所有権を定数時間で呼び出し側へ移動
		events.swap(statusEffectDamageEvents_);
		return events;
	}

	void Base::Kill(DeathReason reason) {
		if (!isActive_) {
			return;
		}

		isActive_ = false;
		status_.currentHealth = 0.0f;
		statusEffectController_.Clear();

		// Map外への落下とPlayer周辺の管理範囲外は撃破として扱わず報酬生成を抑制
		if (reason == DeathReason::Defeated) {
			SpawnDeathReward();
		}
	}

	DamageResult Base::ApplyDamage(float resolvedDamage, bool isCritical, bool playsDamageEffect) {
		DamageResult result;
		if (!isActive_ || !std::isfinite(resolvedDamage) || resolvedDamage <= 0.0f) {
			return result;
		}

		// ダメージの発生源にかかわらずHP減少と死亡判定を一つの経路へ集約
		const float healthBeforeDamage = status_.currentHealth;
		status_.currentHealth = std::max(0.0f, status_.currentHealth - resolvedDamage);
		result.appliedDamage = healthBeforeDamage - status_.currentHealth;
		result.resolvedDamage = resolvedDamage;
		result.wasApplied = result.appliedDamage > 0.0f;
		result.isCritical = result.wasApplied && isCritical;
		result.wasKilled = status_.currentHealth <= 0.0f;
		if (result.wasApplied) {
			StartDamageFlash();
			if (playsDamageEffect) {
				PlayDamageEffect();
			}
		}
		if (result.wasKilled) {
			Kill();
		}

		return result;
	}

	void Base::UpdateStatusEffects(float deltaTime) {
		const StatusEffect::UpdateResult updateResult = statusEffectController_.Update(deltaTime);
		for (std::size_t index = 0; index < updateResult.damageEventCount; ++index) {
			const StatusEffect::DamageEvent& damageEvent = updateResult.damageEvents[index];

			// 複数の継続Damageが同Frameに発生しても死亡後の追加適用を停止
			const DamageResult damageResult = ApplyDamage(damageEvent.damage, false, false);
			if (damageResult.wasApplied) {
				statusEffectDamageEvents_.push_back({
					damageEvent.type,
					damageEvent.sourceWeaponId,
					damageResult.appliedDamage,
					damageResult.resolvedDamage,
					damageResult.wasKilled,
				});
			}
			if (damageResult.wasKilled) {
				break;
			}
		}
	}

	void Base::UpdateProjectileDamageCooldowns(float deltaTime) {
		if (deltaTime <= 0.0f) {
			return;
		}

		// erase後のIteratorを受け取りながら期限切れ要素を安全に除去
		for (auto iterator = projectileDamageCooldowns_.begin(); iterator != projectileDamageCooldowns_.end();) {
			iterator->second -= deltaTime;
			if (iterator->second <= 0.0f) {
				iterator = projectileDamageCooldowns_.erase(iterator);
				continue;
			}

			++iterator;
		}
	}

	void Base::StartDamageFlash() {
		damageFlashRemainingTime_ = kDamageFlashDuration;

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetColor(kDamageFlashColor);
		}
	}

	void Base::PlayDamageEffect() const {
		MadoEngine::EffectSequence::EffectSequencePlayDesc desc;
		desc.rootTransform.translate = transform_.translate;
		desc.sceneType = sceneType_;
		desc.loopOverride = false;
		MadoEngine::EffectSequence::EffectSequenceSystem::GetInstance().Play("EnemyHitEffect", desc);
	}

	void Base::UpdateAppearance(float deltaTime) {
		const Vector4 baseColor = Settings::GetInstance().GetTypeSettings(type_).color;
		if (damageFlashRemainingTime_ > 0.0f) {

			// 通常色のAnimationより被弾Flashを優先して表示
			if (std::isfinite(deltaTime) && deltaTime > 0.0f) {
				damageFlashRemainingTime_ = std::max(0.0f, damageFlashRemainingTime_ - deltaTime);
			}

			if (Model* model = MyModel::TryGet(model_)) {
				model->SetColor(kDamageFlashColor);
			}
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {

			// 基礎色へ現在有効な全状態異常の色係数を重ねて解除時は自動的に元色へ復元
			model->SetColor(ApplyStatusEffectColorMultipliers(baseColor, statusEffectController_));
		}
	}

	void Base::SpawnDeathReward() {

		// Killの重複呼び出しで報酬Dropが複製されないよう一度だけ生成
		if (isDeathRewardSpawned_) {
			return;
		}

		DropObject::Manager::GetInstance().Spawn(DropObject::Type::Exp, transform_.translate);
		DropObject::Manager::GetInstance().Spawn(DropObject::Type::Money, transform_.translate);
		isDeathRewardSpawned_ = true;
	}

	void Base::UpdateEmergence(float deltaTime) {
		const float safeDeltaTime = std::isfinite(deltaTime) ? std::max(0.0f, deltaTime) : 0.0f;
		transform_.translate.y =
			std::min(emergenceTargetY_, transform_.translate.y + kEmergenceSpeed * safeDeltaTime);
		if (transform_.translate.y < emergenceTargetY_ - kEmergenceCompletionEpsilon) {
			return;
		}

		// 地表面到達時に座標誤差を除去して通常移動と衝突判定へ一度だけ遷移
		transform_.translate.y = emergenceTargetY_;
		isEmerging_ = false;
		movement_.Initialize();
		RegisterColliders();
	}

	void Base::RegisterColliders() {
		if (areCollidersRegistered_) {
			return;
		}

		// 移動解決とProjectile被弾で形状とTagを使い分けるためColliderを分離
		MyCollider::RegisterCollider(
			movementColliderName_, CollisionTag::EnemyMovementSphere, &colliderShape_, &transform_.translate, 0.0f);
		MyCollider::RegisterCollider(
			hitColliderName_, CollisionTag::EnemyHitBox, &hitAABB_, &transform_.translate, 0.0f);
		areCollidersRegistered_ = true;
	}

	bool Base::MoveTowardPosition(float deltaTime, const Vector3& targetPosition, float speedMultiplier) {
		const float moveSpeed = status_.moveSpeed * std::max(0.0f, speedMultiplier) *
			statusEffectController_.GetMoveSpeedMultiplier();
		if (movement_.Update(deltaTime, targetPosition, moveSpeed, transform_)) {
			return true;
		}

		Kill(DeathReason::OutsideMap);
		return false;
	}

	bool Base::MoveTowardPlayer(float deltaTime, float speedMultiplier) {
		return MoveTowardPosition(deltaTime, GetTargetPlayerPosition(), speedMultiplier);
	}

	Vector3 Base::GetTargetPlayerPosition() const {
		return targetPlayer_ ? targetPlayer_->GetPosition() : transform_.translate;
	}

	void Base::ApplyModelTransform() {
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetPosition(transform_.translate + GetModelOffset() * bodyScaleMultiplier_);
			model->SetRotation(transform_.rotate);
			model->SetScale(transform_.scale);
		}

		if (Model* eliteMarkerModel = MyModel::TryGet(eliteMarkerModel_)) {
			const AABB& hitCollider = std::get<AABB>(hitAABB_);
			const EliteSettings& eliteSettings = Settings::GetInstance().GetEliteSettings();

			// 種類ごとに異なる被弾Collider上端を頭上基準としてMarker位置を追従
			eliteMarkerModel->SetPosition(
				transform_.translate + Vector3{ 0.0f, hitCollider.max.y + eliteSettings.markerHeightOffset, 0.0f });
			eliteMarkerModel->SetScale(eliteSettings.markerScale);
			eliteMarkerModel->SetVisible(isActive_ && !isEmerging_);
		}
	}

	void Base::CreateEliteMarkerModel() {
		if (bonusType_ != Data::BonusType::Elite) {
			return;
		}

		eliteMarkerModel_ = MyModel::Create(
			modelName_ + "_EliteMarker",
			kEliteMarkerModelAssetName,
			sceneType_,
			MadoEngine::Render::RenderLayer::Enemy);
		if (Model* eliteMarkerModel = MyModel::TryGet(eliteMarkerModel_)) {

			// UI Textureの色と透過を維持するため照明とShadowを無効化したBillboardとして構成
			eliteMarkerModel->SetTexture(kEliteMarkerTextureName);
			eliteMarkerModel->SetUseBillboard(true);
			eliteMarkerModel->SetCastShadow(false);
			eliteMarkerModel->SetReceiveShadow(false);
			eliteMarkerModel->SetLightingEnabled(false);
			eliteMarkerModel->SetColor(kEliteMarkerColor);
		}
	}

	void Base::Release() {

		// Destructorと明示解放の重複呼び出しからColliderと複数Modelを保護
		if (isReleased_) {
			return;
		}

		if (areCollidersRegistered_ && !movementColliderName_.empty()) {
			MyCollider::RemoveCollider(movementColliderName_);
		}
		if (areCollidersRegistered_ && !hitColliderName_.empty()) {
			MyCollider::RemoveCollider(hitColliderName_);
		}
		areCollidersRegistered_ = false;
		if (!modelName_.empty()) {
			MyModel::RequestDestroy(model_);
			model_ = {};
		}
		if (eliteMarkerModel_.IsValid()) {
			MyModel::RequestDestroy(eliteMarkerModel_);
			eliteMarkerModel_ = {};
		}

		isReleased_ = true;
	}

	std::string Base::CreateColliderName(const std::string& prefix) const { return prefix + "_" + std::to_string(enemyId_); }

	std::string Base::CreateModelName() const { return "Enemy_" + std::to_string(enemyId_); }

} // namespace Enemy
