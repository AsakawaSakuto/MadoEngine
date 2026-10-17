#include "Thunder.h"
#include <algorithm>
#include <cmath>

namespace Projectile {
	namespace {
		constexpr float kSpawnHeightOffset = 6.0f;
		constexpr float kBaseModelScale = 0.5f;
		constexpr float kBaseHitboxRadius = 0.5f;
		constexpr float kMinSizeRate = 0.01f;
	}

	Thunder::~Thunder() {
		if (!objectName_.empty()) {

			// Colliderの座標参照を本体の破棄前に解除
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Thunder::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		initialBounceCount_ = remainingBounceCount_;
		hasHitEnemy_ = false;

		// 発射元の位置に依存せず、射撃時点のTargetの真上へ固定
		transform_.translate = targetPosition_ + Vector3{ 0.0f, kSpawnHeightOffset, 0.0f };
		const float safeSizeRate = std::max(sizeRate_, kMinSizeRate);
		const float modelScale = kBaseModelScale * safeSizeRate;
		transform_.scale = { modelScale, modelScale, modelScale };
		SetMoveDirectionTowards(targetPosition_);
		lifeTimer_.Start(lifeTime_, false);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {

			// 周囲の照明に左右されない雷の表示色で落下位置を識別
			model->SetTexture("white2x2");
			model->SetColor({ 1.0f, 0.85f, 0.15f, 1.0f });
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}

		Sphere hitbox;
		hitbox.radius = kBaseHitboxRadius * safeSizeRate;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);
	}

	void Thunder::Update(float deltaTime) {

		// Pause中と不正な時間入力では移動や寿命を進行させない制約
		if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return;
		}

		lifeTimer_.Update(deltaTime);
		if (lifeTimer_.IsFinished()) {
			isDead_ = true;
			return;
		}

		// 落下方向は初期化時だけ設定し、命中後は跳弾で更新された方向を維持
		const float previousHeight = transform_.translate.y;
		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;
		if (!hasHitEnemy_ && previousHeight > targetPosition_.y && transform_.translate.y < targetPosition_.y) {

			// 大きな移動量でもTargetの高さを一度経由して衝突判定の機会を確保
			transform_.translate.y = targetPosition_.y;
		}

		// 外れた雷や跳弾後のProjectileをMap外で残存させない寿命管理
		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void Thunder::OnEnemyHit() {

		// 初回の命中だけで落下状態を終了し、以降の命中では跳弾方向を保持
		if (hasHitEnemy_) {
			return;
		}
		hasHitEnemy_ = true;

		// 共通衝突処理で跳弾が成立していない場合だけ初期の落下方向を停止
		if (remainingBounceCount_ == initialBounceCount_) {
			moveDirection_ = {};
		}
	}
}
