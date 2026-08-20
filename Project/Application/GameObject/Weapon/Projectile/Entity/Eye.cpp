#include "Eye.h"
#include <algorithm>
#include <cmath>
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
		transform_.rotate.y += kRotationSpeed * deltaTime;
		if (Model* model = MyModel::TryGet(model_)) {

			// 所有者の座標を基準に少し上方へオフセットして表示
			transform_.translate = ownerPosition - Vector3(0.0f, 0.45f, 0.0f);
			model->SetTransform(transform_);
		}
		UpdateEffectSequenceTransform();
		UpdateColor(deltaTime);

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

	void Eye::StartEffectSequence() {
		MadoEngine::EffectSequence::EffectSequencePlayDesc desc;
		desc.rootTransform.translate = transform_.translate;
		desc.rootTransform.scale = { sizeRate_, 1.0f, sizeRate_ };
		desc.sceneType = SceneType::Game;
		desc.loopOverride = true;
		effectSequence_.Play("EyeEffect", desc);
	}

	void Eye::UpdateEffectSequenceTransform() {
		Transform3D effectTransform;
		effectTransform.translate = transform_.translate;
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
