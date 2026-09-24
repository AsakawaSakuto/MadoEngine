#include "ToxicBoots.h"
#include <limits>

namespace Projectile {

	ToxicBoots::~ToxicBoots() {
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void ToxicBoots::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		// Playerの移動Collider中心ではなく生成地点の地表へ固定
		transform_.translate = ownerPosition - Vector3(0.0f, 0.45f, 0.0f);
		constexpr float kMaxGroundSearchDistance = std::numeric_limits<float>::max();
		float groundSurfaceY = 0.0f;
		float surfaceY = 0.0f;
		bool foundGround = false;

		if (MyCollider::TryGetGroundSurfaceY(
			ownerPosition, CollisionTag::MapBlock, surfaceY, kMaxGroundSearchDistance)) {
			groundSurfaceY = surfaceY;
			foundGround = true;
		}

		// 通常床と坂が重なる場所では生成地点に近い上側の地表を採用
		if (MyCollider::TryGetGroundSurfaceY(
			ownerPosition, CollisionTag::MapSlope, surfaceY, kMaxGroundSearchDistance) &&
			(!foundGround || surfaceY > groundSurfaceY)) {
			groundSurfaceY = surfaceY;
			foundGround = true;
		}

		if (foundGround) {
			transform_.translate.y = groundSurfaceY + kGroundOffset + MyRand::GetFloat(0.01f, 0.1f);
		}
		transform_.scale = Vector3{ kBaseModelScale, kBaseModelScale, kBaseModelScale } * sizeRate_;

		// 専用Modelが用意されるまでEyeのModelとTextureを仮表示に使用
		model_ = MyModel::Create(objectName_, "Eye", SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("EyeTexture2");
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}

		Sphere hitbox;
		hitbox.radius = kBaseAttackRadius * sizeRate_;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(
			objectName_,
			CollisionTag::PlayerProjectileHitBox,
			&hitbox_,
			&transform_.translate);

		// 設置範囲内の複数Enemyへ接触判定を維持
		disappearsUponCollision_ = false;
		isReductionStarted_ = false;
		reductionTimer_.Reset();
		lifeTimer_.Start(lifeTime_, false);
	}

	void ToxicBoots::Update(float deltaTime) {
		if (lifeTimer_.IsFinished()) {

			// 縮小演出の完了後にProjectileを破棄
			isDead_ = true;
			return;
		}

		if (lifeTimer_.GetProgress() >= kReductionStartRatio && !isReductionStarted_) {

			// Axeと同様に寿命末尾の一割を縮小演出へ割り当て
			isReductionStarted_ = true;
			reductionTimer_.Start(lifeTime_ * (1.0f - kReductionStartRatio), false);
		}

		// 生成位置を維持したままY軸回転だけを更新
		transform_.rotate.y += kRotationSpeed * deltaTime;
		if (Model* model = MyModel::TryGet(model_)) {
			const float reductionProgress = isReductionStarted_ ? reductionTimer_.GetProgress() : 0.0f;
			const Vector3 baseScale = Vector3{ kBaseModelScale, kBaseModelScale, kBaseModelScale } * sizeRate_;

			// 消滅直前だけScaleを線形に縮小して突然の非表示を回避
			transform_.scale = Easing::Lerp(baseScale, Vector3{}, reductionProgress, EaseType::Linear);
			model->SetTransform(transform_);
		}

		lifeTimer_.Update(deltaTime);
		reductionTimer_.Update(deltaTime);

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}
}
