#include "EnemyBase.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "GameObject/Player/Player.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

namespace Enemy {
	namespace {
		constexpr float kDamageFlashDuration = 6.0f / 60.0f;
		constexpr float kEmergenceSpeed = 4.0f;
		constexpr float kEmergenceCompletionEpsilon = 1e-4f;
		constexpr Vector4 kDamageFlashColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	Base::~Base() { Release(); }

	void Base::Initialize(std::uint32_t enemyId, const SpawnDesc& desc) {
		enemyId_ = enemyId;
		status_ = desc.status;
		type_ = desc.type;
		bonusType_ = desc.bonusType;
		sceneType_ = desc.sceneType;
		projectileDamageCooldowns_.clear();
		playerDamageCooldown_ = 0.0f;
		damageFlashRemainingTime_ = 0.0f;
		gamingColor_.Reset();
		isActive_ = status_.currentHealth > 0.0f;
		isEmerging_ = false;
		areCollidersRegistered_ = false;
		isDeathRewardSpawned_ = false;
		isReleased_ = false;
		transform_.translate = desc.position;
		transform_.rotate = {};
		transform_.scale = GetModelScale();
		movement_.Initialize();

		hitAABB_ = CreateHitCollider();
		const Sphere movementCollider = CreateMovementCollider();
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
			model->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
			model->SetTexture("white16x16");
			model->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		ApplyModelTransform();
		OnInitialized();
	}

	void Base::Update(float deltaTime) {

		// 死亡後も残る被弾間隔とDamage演出を破棄まで進行
		UpdateProjectileDamageCooldowns(deltaTime);
		UpdateAppearance(deltaTime);
		playerDamageCooldown_ = std::max(0.0f, playerDamageCooldown_ - std::max(0.0f, deltaTime));

		if (!isActive_ || !targetPlayer_) {
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
		if (!isActive_) {
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
		if (ShouldDisappearOnPlayerCollision()) {
			Kill();
		}

		return true;
	}

	ProjectileDamageResult Base::TakeProjectileDamage(std::uint64_t projectileId, float damage) {
		ProjectileDamageResult result;

		// 不正なProjectile識別子と非有限Damageを状態へ反映しないため入力を検証
		if (!isActive_ || isEmerging_ || projectileId == 0 || !std::isfinite(damage) || damage <= 0.0f) {
			return result;
		}

		if (projectileDamageCooldowns_.contains(projectileId)) {
			return result;
		}

		// 同じProjectileが接触中に毎フレームDamageを与えないよう識別子単位で待機時間を登録
		const float healthBeforeDamage = status_.currentHealth;
		status_.currentHealth = std::max(0.0f, status_.currentHealth - damage);
		projectileDamageCooldowns_.emplace(projectileId, projectileDamageInterval_);
		result.appliedDamage = healthBeforeDamage - status_.currentHealth;
		result.wasApplied = result.appliedDamage > 0.0f;
		result.wasKilled = status_.currentHealth <= 0.0f;
		if (result.wasApplied) {
			StartDamageFlash();
			PlayDamageEffect();
		}
		if (result.wasKilled) {
			Kill();
		}

		return result;
	}

	void Base::Kill(DeathReason reason) {
		if (!isActive_) {
			return;
		}

		isActive_ = false;
		status_.currentHealth = 0.0f;

		// Map外への落下とPlayer周辺の管理範囲外は撃破として扱わず報酬生成を抑制
		if (reason == DeathReason::Defeated) {
			SpawnDeathReward();
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
		const Vector4 baseColor = gamingColor_.Update(deltaTime, 1.0f);
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
			model->SetColor(baseColor);
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
		const float moveSpeed = status_.moveSpeed * std::max(0.0f, speedMultiplier);
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
		Model* model = MyModel::TryGet(model_);
		if (!model) {
			return;
		}

		model->SetPosition(transform_.translate + GetModelOffset());
		model->SetRotation(transform_.rotate);
		model->SetScale(transform_.scale);
	}

	void Base::Release() {

		// Destructorと明示解放の重複呼び出しからColliderとModelを保護
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

		isReleased_ = true;
	}

	std::string Base::CreateColliderName(const std::string& prefix) const { return prefix + "_" + std::to_string(enemyId_); }

	std::string Base::CreateModelName() const { return "Enemy_" + std::to_string(enemyId_); }

} // namespace Enemy
