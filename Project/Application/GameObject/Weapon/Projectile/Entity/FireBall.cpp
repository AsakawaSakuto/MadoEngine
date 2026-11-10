#include "FireBall.h"
#include "Explosion.h"

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
		transform_.translate = ownerPosition_;
		transform_.scale = { 0.5f, 0.5f, 0.5f };
		lifeTimer_.Start(lifeTime_, false);
		StartEffectSequence();

		SetMoveDirectionTowards(targetPosition_);

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
			Explosion::SpawnFromProjectile(*this, sizeRate_, kExplosionDamageRate);
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
		Explosion::SpawnFromProjectile(*this, sizeRate_, kExplosionDamageRate);
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
}
