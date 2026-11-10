#include "Explosion.h"
#include "../ProjectileManager.h"

namespace Projectile {

	void Explosion::SpawnFromProjectile(const IProjectile& source, float radius, float damageRate) {

		// 元の弾が消滅しても攻撃情報と即死の当落を使えるよう生成要求へ値をコピー
		InitializeDesc context{};
		context.sourceWeaponId = source.GetSourceWeaponId();
		context.projectileName = source.GetColliderName();
		context.ownerPosition = source.GetPosition();
		context.damage = source.GetDamage();
		context.instantKillChance = source.GetInstantKillChance();
		context.instantKillRoll = source.GetInstantKillRoll();
		context.criticalChance = source.GetCriticalChance();
		context.criticalDamage = source.GetCriticalDamage();
		context.knockbackPower = source.GetKnockbackPower();
		context.statusEffects = source.GetStatusEffects();
		context.explosionDamageDecreaseRate = damageRate;
		context.explosionRadius = radius;
		Manager::GetInstance().AddProjectile(Type::Explosion, context);

		MadoEngine::Particle::PlayDesc desc;
		desc.transform.translate = context.ownerPosition;
		desc.sceneType = SceneType::Game;
		desc.loopOverride = false;
		MyParticle3d::Play("DefaultExplosion", desc);
	}
	
	Explosion::~Explosion() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
		}
	}

	void Explosion::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId) + "_Explosion";
		InitializeCommonProperties(context, objectName_);

		transform_.translate = ownerPosition_;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		damage_ = context.damage * context.explosionDamageDecreaseRate;

		Sphere hitbox;
		hitbox.center = ownerPosition_;
		hitbox.radius = context.explosionRadius;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		// 短い寿命中に範囲内の複数EnemyへHitできる衝突設定
		disappearsUponCollision_ = false;

		lifeTime_ = 0.1f;
		lifeTimer_.Start(lifeTime_, false);
	}

	void Explosion::Update(float deltaTime) {

		if (lifeTimer_.IsFinished()) {

			// 一Frame限りではなく衝突収集可能な短時間を維持してから破棄
			isDead_ = true;
		}

		lifeTimer_.Update(deltaTime);
		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}
}
