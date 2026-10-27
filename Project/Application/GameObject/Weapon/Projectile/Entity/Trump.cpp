#include "Trump.h"
#include "Render/Object/3d/RibbonEffect/MyRibbonEffect3d.h"
#include <algorithm>
#include <cmath>

namespace Projectile {

	Trump::~Trump() {

		// 本体消滅後は新しい軌跡の生成を停止し、残った部分だけを寿命に従って消去
		MyRibbonEffect3d::Stop(trail_, MadoEngine::Ribbon::RibbonStopMode::Finish);
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Trump::Initialize(InitializeDesc context) {

		// 再初期化時に前の軌跡へ新しい発射位置を接続しないよう既存Trailを破棄
		MyRibbonEffect3d::Stop(trail_, MadoEngine::Ribbon::RibbonStopMode::Immediate);
		trail_ = {};
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		baseDamage_ = damage_;

		transform_.translate = ownerPosition_;
		const float safeSizeRate = [&]() {
			if (std::isfinite(sizeRate_)) {
				return (std::max(sizeRate_, kMinSizeRate));
			} else {
				return (kMinSizeRate);
			}
		}();
		const float size = kBaseSize * safeSizeRate;
		transform_.scale = { size, size, size };
		SetMoveDirectionTowards(targetPosition_);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}

		Sphere hitbox;
		hitbox.radius = size;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		const float safeLifeTime = [&]() {
			if (std::isfinite(lifeTime_)) {
				return (std::max(lifeTime_, kMinLifeTime));
			} else {
				return (kMinLifeTime);
			}
		}();

		lifeTimer_.Start(safeLifeTime, false);

		// Projectileの中心からWorld座標の履歴を開始し、Modelの拡縮に依存しない幅で表示
		MadoEngine::Ribbon::RibbonEffectPlayDesc trailDesc;
		trailDesc.transform.translate = transform_.translate;
		trailDesc.sceneType = SceneType::Game;
		trailDesc.loopOverride = true;
		trail_ = MyRibbonEffect3d::Play("TrumpTrail", trailDesc);
	}

	void Trump::Update(float deltaTime) {
		lifeTimer_.Update(deltaTime);
		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;

		// 寿命切れやMap外への移動で失効したProjectileは軌跡の追従対象から除外
		if (lifeTimer_.IsFinished() ||
			!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}

		// 跳弾を含む移動後の中心座標へTrailを追従し、履歴の寿命管理はRibbon側へ委譲
		if (trail_.HasValue()) {
			Transform3D trailTransform;
			trailTransform.translate = transform_.translate;
			MyRibbonEffect3d::SetTransform(trail_, trailTransform);
		}

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void Trump::OnEnemyHit() {

		// 貫通数が後から設定された場合も、前回の命中結果を引き継がず命中ごとに再抽選
		damage_ = baseDamage_ + static_cast<float>(
			MyRand::GetInt(kMinRandomDamageBonus, kMaxRandomDamageBonus));
		if (MyRand::GetInt(kInstantKillRollMin, kInstantKillRollMax) == kInstantKillRollSuccess) {
			damageMode_ = DamageMode::RemainingHealth;
		} else {
			damageMode_ = DamageMode::Standard;
		}
	}
}
