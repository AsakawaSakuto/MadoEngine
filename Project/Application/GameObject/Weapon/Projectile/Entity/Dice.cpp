#include "Dice.h"
#include "Explosion.h"
#include "GameObject/Combat/ProjectileDamage.h"

namespace Projectile {

	Dice::~Dice() {

		// Colliderを先に解除し、破棄後の座標や形状を参照させないよう寿命を同期
		if (!colliderName_.empty()) {
			MyCollider::RemoveCollider(colliderName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Dice::Initialize(InitializeDesc context) {
		const std::string objectName = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName);

		// 同じ弾から生まれるすべての爆発で追加ダメージと即死の当落を共有できるよう射出時に確定
		damage_ += static_cast<float>(MyRand::GetInt(kMinRandomDamageBonus, kMaxRandomDamageBonus));
		instantKillRoll_ = Combat::RollInstantKill(instantKillChance_);
		transform_.translate = ownerPosition_;
		transform_.scale = { kModelScale, kModelScale, kModelScale };
		SetMoveDirectionTowards(targetPosition_);
		lifeTimer_.Start(lifeTime_, false);

		model_ = MyModel::Create(objectName, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
			model->SetCastShadow(false);
		}

		// 本体の回転や爆発半径の強化に左右されない球形で着弾を検知
		Sphere hitbox;
		hitbox.radius = kHitRadius;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void Dice::Update(float deltaTime) {
		lifeTimer_.Update(deltaTime);
		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;
		transform_.rotate.x += kRotationSpeed * deltaTime;
		transform_.rotate.z += kRotationSpeed * deltaTime;

		// 敵に触れずに寿命が尽きた弾や範囲外の弾は着弾判定から除外
		if (lifeTimer_.IsFinished() ||
			!MyCollider::IsHitWithTag(colliderName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}
		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void Dice::OnEnemyHit() {
		Explosion::SpawnFromProjectile(*this, sizeRate_, kExplosionDamageRate);
	}
}
