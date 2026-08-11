#include "FireBall.h"
#include "../../../Map/MapLimit.h"
#include "../ProjectileManager.h"
#include <cmath>

namespace Projectile {

	FireBall::~FireBall() {
		StopEffectSequence();

		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void FireBall::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		// Modelを持たずEffect Sequenceを本体表示として使用
		transform_.translate = ownerPosition;
		transform_.scale = { 0.5f, 0.5f, 0.5f };
		lifeTimer_.Start(lifeTime_, false);
		StartEffectSequence();

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
		UpdateEffectSequenceTransform();

		// Map外への到達と寿命切れのどちらでも終端Explosionを生成
		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox) || lifeTimer_.IsFinished()) {
			SpawnExplosion();
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}

	void FireBall::OnEnemyHit() {

		// 本体の貫通状態とは独立して接触地点へ範囲攻撃を生成
		SpawnExplosion();
	}

	void FireBall::StartEffectSequence() {
		MadoEngine::EffectSequence::EffectSequencePlayDesc desc;
		desc.rootTransform.translate = transform_.translate;
		desc.sceneType = SceneType::Game;
		desc.loopOverride = true;
		effectSequence_.Play("FireBall", desc);
	}

	void FireBall::UpdateEffectSequenceTransform() {
		Transform3D effectTransform;
		effectTransform.translate = transform_.translate;

		if (!effectSequence_.SetTransform(effectTransform)) {

			// Sequence側でHandleが失効していた場合は追従表現を再生成
			StartEffectSequence();
		}
	}

	void FireBall::StopEffectSequence() {
		effectSequence_.Stop(MadoEngine::EffectSequence::EffectSequenceStopMode::Immediate);
	}

	void FireBall::SpawnExplosion() {

		// Manager走査中でも安全に追加できる生成要求としてExplosionを登録
		Projectile::InitializeDesc context{};
		context.sourceWeaponId = sourceWeaponId_;
		context.projectileName = objectName_;
		context.ownerPosition = transform_.translate;
		context.damage = damage_;
		context.explotionDamageDecreaseRate = 0.75f;
		context.explosionRadius = sizeRate_;
		Projectile::Manager::GetInstance().AddProjectile(Projectile::Type::Explosion, context);

		MadoEngine::Particle::PlayDesc desc;
		desc.transform.translate = transform_.translate;
		desc.sceneType = SceneType::Game;
		desc.loopOverride = false;
		auto handle = MyParticle3d::Play("DefaultExplosion", desc);
	}
}
