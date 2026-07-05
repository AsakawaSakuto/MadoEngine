#include "Enemy.h"
#include "Object/Player/Player.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kDirectionEpsilon = 1e-5f;        // 方向ベクトルの長さがこの値以下の場合は正規化を行わない
	constexpr float kSlopeSnapDistance = 1.0f;        // Slopeの中心Y座標にスナップする距離の閾値
	constexpr float kDeleteGroundY = 0.0f;            // このY座標以下に落下した場合は削除対象とする
	constexpr float kSideClimbSpeed = 5.0f;           // 側面上昇補助の速度
	constexpr float kMaxSideClimbHeight = 100.0f;     // 側面上昇補助の最大上昇量
	constexpr float kBlockedProgressRate = 0.35f;     // 側面で止められた場合の前進量の減衰率
	constexpr float kSideClimbCrestGraceTime = 0.25f; // 側面上昇補助の頂点での猶予時間
	constexpr float kSideClimbCrestSpeedScale = 0.6f; // 側面上昇補助の頂点での速度倍率

	/// @brief 水平方向の長さの2乗を取得します。
	/// @param value 対象のベクトルです。
	/// @return XZ平面での長さの2乗です。
	float GetHorizontalLengthSq(const Vector3& value) {
		return value.x * value.x + value.z * value.z;
	}
}

Enemy::~Enemy() {
	Release();
}

void Enemy::Initialize(uint32_t enemyId, const Vector3& spawnPosition, SceneType sceneType) {
	enemyId_ = enemyId;
	transform_.translate = spawnPosition;
	transform_.SetAllScale(0.5f);

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

	MyCollider::RegisterCollider(movementColliderName_, CollisionTag::EnemyMovementSphere, &colliderShape_, &transform_.translate, 0.0f);
	MyCollider::RegisterCollider(hitColliderName_, CollisionTag::EnemyHitBox, &hitAABB_, &transform_.translate, 0.0f);

	model_ = MyModel::Create(modelName_, "enemy", sceneType); // cube
	if (model_) {
		model_->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
		model_->SetTexture("white16x16");
		model_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	UpdateModelTransform();
}

void Enemy::Update(float deltaTime) {
	lastDeltaTime_ = deltaTime;
	lastDesiredHorizontalMove_ = { 0.0f, 0.0f, 0.0f };
	lastMoveStartPosition_ = transform_.translate;

	if (!isActive_ || !targetPlayer_) {
		return;
	}

	const Vector3 direction = GetDirectionToPlayer();
	lastDesiredHorizontalMove_ = { direction.x * moveSpeed_ * deltaTime, 0.0f, direction.z * moveSpeed_ * deltaTime };
	transform_.translate.x += lastDesiredHorizontalMove_.x;
	transform_.translate.z += lastDesiredHorizontalMove_.z;

	if (!isGrounded_) {
		velocityY_ -= gravity_ * deltaTime;
	}
	transform_.translate.y += velocityY_ * deltaTime;

	transform_.translate.x = std::clamp(transform_.translate.x, -7.5f, 292.5f);
	transform_.translate.y = std::clamp(transform_.translate.y, 0.0f, 100.0f);
	transform_.translate.z = std::clamp(transform_.translate.z, -7.5f, 292.5f);

	if (transform_.translate.y <= kDeleteGroundY) {
		Kill();
		return;
	}

	if (GetHorizontalLengthSq(direction) > kDirectionEpsilon) {
		transform_.rotate.y = std::atan2(direction.x, direction.z);
	}
}

void Enemy::ResolveAfterCollision() {
	if (!isActive_) {
		return;
	}

	const bool isGroundContact = MyCollider::IsGroundContact(movementColliderName_, CollisionTag::MapBlock);
	const bool isSlopeGroundContact = MyCollider::IsSlopeGroundContact(movementColliderName_, CollisionTag::MapSlope);
	isGrounded_ = isGroundContact || isSlopeGroundContact;

	if (isGrounded_ && velocityY_ < 0.0f) {
		velocityY_ = 0.0f;
	}

	if (isSlopeGroundContact && velocityY_ <= 0.0f) {
		float slopeCenterY = 0.0f;
		if (MyCollider::TryGetSlopeGroundCenterY(movementColliderName_, CollisionTag::MapSlope, slopeCenterY, kSlopeSnapDistance) &&
			transform_.translate.y > slopeCenterY) {
			transform_.translate.y = slopeCenterY;
		}
	}

	ApplySideClimbAssist(lastDeltaTime_, isGroundContact, isSlopeGroundContact);

	UpdateModelTransform();
}

bool Enemy::IsHitPlayer() const {
	if (!isActive_) {
		return false;
	}

	return MyCollider::IsHitWithTag(hitColliderName_, CollisionTag::PlayerHitBox);
}

bool Enemy::IsHitPlayerProjectile() const {
	if (!isActive_) {
		return false;
	}

	return MyCollider::IsHitWithTag(hitColliderName_, CollisionTag::PlayerProjectileHitBox);
}

void Enemy::Release() {
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
		MyModel::Destroy(modelName_);
	}

	isReleased_ = true;
}

std::string Enemy::CreateColliderName(const std::string& prefix) const {
	return prefix + "_" + std::to_string(enemyId_);
}

std::string Enemy::CreateModelName() const {
	return "Enemy_" + std::to_string(enemyId_);
}

Vector3 Enemy::GetDirectionToPlayer() const {
	if (!targetPlayer_) {
		return { 0.0f, 0.0f, 0.0f };
	}

	Vector3 direction = targetPlayer_->GetPosition() - transform_.translate;
	direction.y = 0.0f;

	const float lengthSq = GetHorizontalLengthSq(direction);
	if (lengthSq < kDirectionEpsilon) {
		return { 0.0f, 0.0f, 0.0f };
	}

	const float invLength = 1.0f / std::sqrt(lengthSq);
	return { direction.x * invLength, 0.0f, direction.z * invLength };
}

void Enemy::ApplySideClimbAssist(float deltaTime, bool isGroundContact, bool isSlopeGroundContact) {
	const bool isSideBlocked = IsSideBlockedThisFrame();
	const bool canStartClimb = isGroundContact || isSlopeGroundContact;
	const bool hasDesiredMove = GetHorizontalLengthSq(lastDesiredHorizontalMove_) > kDirectionEpsilon;
	const bool canContinueToCrest =
		isSideClimbing_ &&
		!canStartClimb &&
		hasDesiredMove &&
		sideClimbCrestTimer_ < kSideClimbCrestGraceTime;

	if (!isSideBlocked && !canContinueToCrest) {
		ResetSideClimbAssist();
		return;
	}

	if (isSideBlocked && !canStartClimb && !isSideClimbing_) {
		ResetSideClimbAssist();
		return;
	}

	if (!isSideClimbing_) {
		isSideClimbing_ = true;
		sideClimbBaseY_ = transform_.translate.y;
		sideClimbAmount_ = 0.0f;
	}

	if (isSideBlocked) {
		sideClimbCrestTimer_ = 0.0f;
	} else {
		sideClimbCrestTimer_ += deltaTime;
	}

	sideClimbAmount_ = std::max(sideClimbAmount_, transform_.translate.y - sideClimbBaseY_);
	const float remainClimbHeight = kMaxSideClimbHeight - sideClimbAmount_;
	if (remainClimbHeight <= 0.0f) {
		velocityY_ = 0.0f;
		return;
	}

	const float climbSpeed = isSideBlocked ? kSideClimbSpeed : kSideClimbSpeed * kSideClimbCrestSpeedScale;
	const float climbStep = std::min(climbSpeed * deltaTime, remainClimbHeight);
	transform_.translate.y += climbStep;
	sideClimbAmount_ += climbStep;
	velocityY_ = 0.0f;
	isGrounded_ = true;
}

void Enemy::ResetSideClimbAssist() {
	isSideClimbing_ = false;
	sideClimbBaseY_ = transform_.translate.y;
	sideClimbAmount_ = 0.0f;
	sideClimbCrestTimer_ = 0.0f;
}

bool Enemy::IsSideBlockedThisFrame() const {
	const float desiredLengthSq = GetHorizontalLengthSq(lastDesiredHorizontalMove_);
	if (desiredLengthSq <= kDirectionEpsilon) {
		return false;
	}

	const float desiredLength = std::sqrt(desiredLengthSq);
	const Vector3 actualHorizontalMove = {
		transform_.translate.x - lastMoveStartPosition_.x,
		0.0f,
		transform_.translate.z - lastMoveStartPosition_.z
	};
	const float actualProgress =
		(actualHorizontalMove.x * lastDesiredHorizontalMove_.x + actualHorizontalMove.z * lastDesiredHorizontalMove_.z) /
		desiredLength;

	return actualProgress < desiredLength * kBlockedProgressRate;
}

void Enemy::UpdateModelTransform() {
	if (!model_) {
		return;
	}

	model_->SetPosition(transform_.translate + Vector3{ 0.0f, -0.5f, 0.0f });
	model_->SetRotation(transform_.rotate);
	model_->SetScale(transform_.scale);
}
