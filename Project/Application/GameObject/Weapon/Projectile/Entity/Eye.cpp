#include "Eye.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace Projectile {

	Eye::~Eye() {
		StopEffectSequence();

		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Eye::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		rotationYaw_ = 0.0f;
		groundNormal_ = { 0.0f, 1.0f, 0.0f };

		Sphere hitbox;
		hitbox_ = hitbox;
		SynchronizePersistentState(context);
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("EyeTexture2");
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}
		StartEffectSequence();
		colorAnimationTimer_.Start(kColorCycleDuration, true);
		UpdateColor(0.0f);

		// 接触中の全Enemyへ既存のProjectile別再ダメージ間隔を適用
		disappearsUponCollision_ = false;
	}

	void Eye::Update(float deltaTime) {

		// 常時展開中であることを視認できるよう所有者の周囲でEyeを回転
		rotationYaw_ += kRotationSpeed * deltaTime;
		transform_.rotate = CreateGroundAlignedRotation(rotationYaw_, groundNormal_);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}
		UpdateEffectSequenceTransform();
		UpdateColor(deltaTime);

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void Eye::SynchronizePersistentState(const InitializeDesc& context) {
		ownerPosition = context.ownerPosition;
		UpdateGroundPosition();
		damage_ = context.damage;
		knockbackPower_ = context.knockbackPower;

		// 不正な倍率による反転やゼロ半径を防ぎつつ強化値を攻撃範囲へ即時反映
		sizeRate_ = std::isfinite(context.sizeRate)
			? std::max(context.sizeRate, kMinSizeRate)
			: kMinSizeRate;
		Sphere& hitbox = std::get<Sphere>(hitbox_);
		hitbox.radius = kBaseAttackRadius * sizeRate_;

		const float modelScale = kBaseModelScale * sizeRate_;
		transform_.scale = { modelScale, modelScale, modelScale };
	}

	void Eye::UpdateGroundPosition() {
		transform_.translate.x = ownerPosition.x;
		transform_.translate.z = ownerPosition.z;

		constexpr float kMaxGroundSearchDistance = std::numeric_limits<float>::max();
		float groundSurfaceY = 0.0f;
		float surfaceY = 0.0f;
		bool foundGround = false;
		groundNormal_ = { 0.0f, 1.0f, 0.0f };

		if (MyCollider::TryGetGroundSurfaceY(
			ownerPosition, CollisionTag::MapBlock, surfaceY, kMaxGroundSearchDistance)) {
			groundSurfaceY = surfaceY;
			foundGround = true;
		}

		// 通常床と坂が重なる場所では所有者に近い上側の地表を採用
		Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
		if (MyCollider::TryGetGroundSurface(
			ownerPosition, CollisionTag::MapSlope, surfaceY, slopeNormal, kMaxGroundSearchDistance) &&
			(!foundGround || surfaceY > groundSurfaceY)) {
			groundSurfaceY = surfaceY;
			groundNormal_ = slopeNormal;
			foundGround = true;
		}

		if (foundGround) {
			transform_.translate.y = groundSurfaceY + kGroundOffset;
			hasGroundPosition_ = true;
		} else if (!hasGroundPosition_) {

			// Map生成前など地表Colliderが未登録の初回だけ従来位置へ退避
			transform_.translate.y = ownerPosition.y - kOwnerGroundOffset;
		}

		transform_.rotate = CreateGroundAlignedRotation(rotationYaw_, groundNormal_);
	}

	void Eye::StartEffectSequence() {
		MadoEngine::EffectSequence::EffectSequencePlayDesc desc;
		desc.rootTransform.translate = transform_.translate;
		desc.rootTransform.rotate = transform_.rotate;
		desc.rootTransform.scale = { sizeRate_, 1.0f, sizeRate_ };
		desc.sceneType = SceneType::Game;
		desc.loopOverride = true;
		effectSequence_.Play("EyeEffect", desc);
	}

	void Eye::UpdateEffectSequenceTransform() {
		Transform3D effectTransform;
		effectTransform.translate = transform_.translate;
		effectTransform.rotate = transform_.rotate;
		effectTransform.scale = { sizeRate_, 1.0f, sizeRate_ };

		if (!effectSequence_.SetTransform(effectTransform)) {

			// Sequence側でHandleが失効していた場合は常時展開表現を再生成
			StartEffectSequence();
		}
	}

	void Eye::StopEffectSequence() {
		effectSequence_.Stop(MadoEngine::EffectSequence::EffectSequenceStopMode::Immediate);
	}

	void Eye::UpdateColor(float deltaTime) {
		const float safeDeltaTime = std::isfinite(deltaTime)
			? std::max(deltaTime, 0.0f)
			: 0.0f;
		colorAnimationTimer_.Update(safeDeltaTime);

		// GameTimerのループ進捗をCos波へ変換して往復端の色変化を平滑化
		const float cycleRatio = colorAnimationTimer_.GetProgress();
		const float blendFactor = 0.5f - 0.5f * std::cos(
			cycleRatio * 2.0f * std::numbers::pi_v<float>
		);
		const Vector4 color = kColorStart + (kColorEnd - kColorStart) * blendFactor;

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetColor(color);
		}
		effectSequence_.SetColorMultiplier(color);
	}
}
