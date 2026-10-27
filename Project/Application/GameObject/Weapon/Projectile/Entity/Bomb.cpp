#include "Bomb.h"
#include "../ProjectileManager.h"
#include "Audio/MyAudio.h"
#include <algorithm>
#include <cmath>

namespace Projectile {

	Bomb::~Bomb() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Bomb::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		explosionDamage_ = context.damage;
		damage_ = 0.0f; // 爆発でダメージを与えるため、Bomb自体のダメージは0に設定

		startPosition_ = ownerPosition_;
		transform_.translate = startPosition_;
		transform_.scale = Vector3{ 0.5f, 0.5f, 0.5f };

		const Vector3 toTarget = targetPosition_ - startPosition_;
		const float distance = toTarget.Length();

		flightDuration_ = distance / moveSpeed_;

		const float horizontalDistance = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
		arcHeight_ = std::clamp(horizontalDistance * kArcHeightDistanceRate, kMinArcHeight, kMaxArcHeight);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("BombModelTexture");
			model->SetTransform(transform_);
			model->SetCastShadow(true);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(true);
		}

		Sphere hitbox;
		hitbox.radius = 1.0f;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		flightElapsedTime_ = 0.0f;
		arrivalElapsedTime_ = 0.0f;
		hasArrived_ = false;
	}

	void Bomb::Update(float deltaTime) {

		if (!hasArrived_) {
			flightElapsedTime_ += deltaTime;
			const float progress = std::clamp(
				flightElapsedTime_ / flightDuration_, 0.0f, 1.0f);

			// 直線補間へ4t(1-t)の高さを加え、両端を固定した上向きの放物線を形成
			transform_.translate = Math::Lerp(startPosition_, targetPosition_, progress);
			transform_.translate.y += 4.0f * arcHeight_ * progress * (1.0f - progress);
			transform_.rotate.y += kRotationSpeed * deltaTime;

			if (progress >= 1.0f) {
				hasArrived_ = true;
			}
		} else {

			// Projectile更新より前に行われるEnemy衝突判定へ着地点を一時的に公開
			arrivalElapsedTime_ += deltaTime;
			if (arrivalElapsedTime_ >= kArrivalHoldDuration) {
				isDead_ = true;
				return;
			}
		}

		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void Bomb::OnDestroy() {

		// 爆発音と重ならないよう、爆発生成前にBombの再生を停止
		if (MyAudio::IsLoaded("Bomb")) {
			MyAudio::Stop("Bomb");
		}

		// 消滅確定時だけ生成し、敵命中と更新終了が重なっても爆発を一回に限定
		Projectile::InitializeDesc context{};
		context.sourceWeaponId = sourceWeaponId_;
		context.projectileName = objectName_;
		context.createSoundKey = "BombExplosion";
		context.ownerPosition = transform_.translate;
		context.damage = explosionDamage_;
		context.criticalChance = criticalChance_;
		context.criticalDamage = criticalDamage_;
		context.knockbackPower = knockbackPower_;
		context.statusEffects = statusEffects_;
		context.explosionDamageDecreaseRate = 1.0f;
		context.explosionRadius = sizeRate_;
		Projectile::Manager::GetInstance().AddProjectile(Projectile::Type::Explosion, context);

		// FireBallと同じ爆発表現を消滅地点に一回だけ再生
		MadoEngine::Particle::PlayDesc desc;
		desc.transform.translate = transform_.translate;
		desc.sceneType = SceneType::Game;
		desc.loopOverride = false;
		MyParticle3d::Play("DefaultExplosion", desc);
	}
}
