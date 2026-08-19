#include "Eye.h"
#include <algorithm>
#include <cmath>

namespace Projectile {

	Eye::~Eye() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Eye::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		Sphere hitbox;
		hitbox_ = hitbox;
		SynchronizePersistentState(context);
		MyCollider::RegisterCollider(
			objectName_,
			CollisionTag::PlayerProjectileHitBox,
			&hitbox_,
			&transform_.translate);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("EyeTexture2");
			model->SetTransform(transform_);
			model->SetColor(Vector4(1.0f, 0.0f, 1.0f, 1.0f)); // 紫色に設定
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
		}

		// 接触中の全Enemyへ既存のProjectile別再ダメージ間隔を適用
		disappearsUponCollision_ = false;
	}

	void Eye::Update(float deltaTime) {

		// 常時展開中であることを視認できるよう所有者の周囲でEyeを回転
		transform_.rotate.y += kRotationSpeed * deltaTime;
		if (Model* model = MyModel::TryGet(model_)) {
			// 所有者の座標を基準に少し上方へオフセットして表示
			transform_.translate = ownerPosition + Vector3(0.0f, 0.1f, 0.0f);
			model->SetTransform(transform_);
		}

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void Eye::SynchronizePersistentState(const InitializeDesc& context) {
		ownerPosition = context.ownerPosition;
		transform_.translate = ownerPosition;
		damage_ = context.damage;

		// 不正な倍率による反転やゼロ半径を防ぎつつ強化値を攻撃範囲へ即時反映
		sizeRate_ = std::isfinite(context.sizeRate)
			? std::max(context.sizeRate, kMinSizeRate)
			: kMinSizeRate;
		Sphere& hitbox = std::get<Sphere>(hitbox_);
		hitbox.radius = kBaseAttackRadius * sizeRate_;

		const float modelScale = kBaseModelScale * sizeRate_;
		transform_.scale = { modelScale, modelScale, modelScale };
	}
}
