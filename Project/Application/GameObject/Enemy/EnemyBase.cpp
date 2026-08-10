#include "EnemyBase.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "GameObject/Player/Player.h"
#include <algorithm>
#include <cmath>

namespace Enemy {
	namespace {
		constexpr float kDamageFlashDuration = 6.0f / 60.0f;
		constexpr Vector4 kDamageFlashColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	Base::~Base() { Release(); }

	void Base::Initialize(std::uint32_t enemyId, const SpawnDesc& desc) {
		enemyId_ = enemyId;
		status_ = desc.status;
		type_ = desc.type;
		projectileDamageCooldowns_.clear();
		damageFlashRemainingTime_ = 0.0f;
		gamingColor_.Reset();
		isActive_ = status_.currentHealth > 0.0f;
		isDeathRewardSpawned_ = false;
		isReleased_ = false;
		transform_.translate = desc.position;
		transform_.rotate = {};
		transform_.SetAllScale(0.5f);
		movement_.Initialize();

		AABB aabb;
		aabb.min = { -0.5f, 0.0f, -0.5f };
		aabb.max = { 0.5f, 2.0f, 0.5f };
		hitAABB_ = aabb;

		Sphere sphere;
		sphere.radius = 0.5f;
		colliderShape_ = sphere;

		movementColliderName_ = CreateColliderName("EnemyMovementSphere");
		hitColliderName_ = CreateColliderName("EnemyHitBox");
		modelName_ = CreateModelName();

		MyCollider::RegisterCollider(movementColliderName_, CollisionTag::EnemyMovementSphere, &colliderShape_, &transform_.translate,
									 0.0f);
		MyCollider::RegisterCollider(hitColliderName_, CollisionTag::EnemyHitBox, &hitAABB_, &transform_.translate, 0.0f);

		model_ = MyModel::Create(modelName_, "enemy", desc.sceneType);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
			model->SetTexture("white16x16");
			model->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		ApplyModelTransform();
	}

	void Base::Update(float deltaTime) {
		UpdateProjectileDamageCooldowns(deltaTime);
		UpdateAppearance(deltaTime);

		if (!isActive_ || !targetPlayer_) {
			return;
		}

		if (!movement_.Update(deltaTime, targetPlayer_->GetPosition(), status_.moveSpeed, transform_)) {
			Kill();
		}
	}

	void Base::ResolveAfterCollision() {
		if (!isActive_) {
			return;
		}

		movement_.ResolveAfterCollision(movementColliderName_, transform_);
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
		if (!isActive_) {
			return false;
		}

		return MyCollider::IsHitWithTag(hitColliderName_, CollisionTag::PlayerHitBox);
	}

	ProjectileDamageResult Base::TakeProjectileDamage(std::uint64_t projectileId, float damage) {
		ProjectileDamageResult result;
		if (!isActive_ || projectileId == 0 || !std::isfinite(damage) || damage <= 0.0f) {
			return result;
		}

		if (projectileDamageCooldowns_.contains(projectileId)) {
			return result;
		}

		const float healthBeforeDamage = status_.currentHealth;
		status_.currentHealth = std::max(0.0f, status_.currentHealth - damage);
		projectileDamageCooldowns_.emplace(projectileId, projectileDamageInterval_);
		result.appliedDamage = healthBeforeDamage - status_.currentHealth;
		result.wasApplied = result.appliedDamage > 0.0f;
		result.wasKilled = status_.currentHealth <= 0.0f;
		if (result.wasApplied) {
			StartDamageFlash();
		}
		if (result.wasKilled) {
			Kill();
		}

		return result;
	}

	void Base::Kill() {
		if (!isActive_) {
			return;
		}

		isActive_ = false;
		status_.currentHealth = 0.0f;
		SpawnDeathReward();
	}

	void Base::UpdateProjectileDamageCooldowns(float deltaTime) {
		if (deltaTime <= 0.0f) {
			return;
		}

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

	void Base::UpdateAppearance(float deltaTime) {
		const Vector4 baseColor = gamingColor_.Update(deltaTime, 1.0f);
		if (damageFlashRemainingTime_ > 0.0f) {
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
		if (isDeathRewardSpawned_) {
			return;
		}

		DropObject::Manager::GetInstance().Spawn(DropObject::Type::Exp, transform_.translate);
		isDeathRewardSpawned_ = true;
	}

	void Base::ApplyModelTransform() {
		Model* model = MyModel::TryGet(model_);
		if (!model) {
			return;
		}

		model->SetPosition(transform_.translate + Vector3{ 0.0f, -0.5f, 0.0f });
		model->SetRotation(transform_.rotate);
		model->SetScale(transform_.scale);
	}

	void Base::Release() {
		if (isReleased_) {
			return;
		}

		if (!movementColliderName_.empty()) {
			MyCollider::RemoveCollider(movementColliderName_);
		}
		if (!hitColliderName_.empty()) {
			MyCollider::RemoveCollider(hitColliderName_);
		}
		if (!modelName_.empty()) {
			MyModel::RequestDestroy(model_);
			model_ = {};
		}

		isReleased_ = true;
	}

	std::string Base::CreateColliderName(const std::string& prefix) const { return prefix + "_" + std::to_string(enemyId_); }

	std::string Base::CreateModelName() const { return "Enemy_" + std::to_string(enemyId_); }

} // namespace Enemy
