#include "FireBall.h"
#include "../../../Map/MapLimit.h"
#include "../ProjectileManager.h"
#include <cmath>

namespace Projectile {

	FireBall::~FireBall() {
		StopParticle();

		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::Destroy(objectName_);
		}
	}

	void FireBall::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		//model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };
		lifeTimer_.Start(lifeTime_, false);
		StartParticle();

		SetMoveDirectionTowards(targetPosition);

		AABB hitbox;
		float size = 0.5f;
		hitbox.min = { -size, -size, -size };
		hitbox.max = { size, size, size };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void FireBall::Update(float deltaTime) {
		lifeTimer_.Update(deltaTime);

		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;
		UpdateParticleTransform();

		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox) || lifeTimer_.IsFinished()) {
			SpawnExplosion();
			isDead_ = true;
			return;
		}

		if (model_) {
			model_->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}

	void FireBall::OnEnemyHit() {
		SpawnExplosion();
	}

	void FireBall::StartParticle() {
		MadoEngine::Particle::PlayDesc desc;
		desc.transform.translate = transform_.translate;
		desc.sceneType = SceneType::Game;
		desc.loopOverride = true;
		particleHandle_ = MyParticle3d::Play("FireBall", desc);
	}

	void FireBall::UpdateParticleTransform() {
		Transform3D particleTransform;
		particleTransform.translate = transform_.translate;
		if (!particleHandle_.HasValue()) {
			return;
		}

		if (!MyParticle3d::SetTransform(particleHandle_, particleTransform)) {
			StartParticle();
		}
	}

	void FireBall::StopParticle() {
		if (!particleHandle_.HasValue()) {
			return;
		}

		MyParticle3d::Stop(particleHandle_, MadoEngine::Particle::StopMode::Immediate);
		particleHandle_ = {};
	}

	void FireBall::SpawnExplosion() {
		Projectile::InitializeDesc context{};
		context.projectileName = objectName_;
		context.ownerPosition = transform_.translate;
		context.damage = damage_;
		context.explotionDamageDecreaseRate = 0.75f;
		context.explosionRadius = sizeRate_;
		Projectile::Manager::GetInstance().AddProjectile(Projectile::Type::Explosion, context);

		MadoEngine::Particle::PlayDesc desc;
		desc.transform.translate = transform_.translate;
		desc.sceneType = SceneType::Test;
		desc.loopOverride = false;
		auto handle = MyParticle3d::Play("HitEffect", desc);
	}

}
