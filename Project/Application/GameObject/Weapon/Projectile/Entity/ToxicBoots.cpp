#include "ToxicBoots.h"
#include "Render/Object/3d/Particle/MyParticle3d.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace Projectile {

	ToxicBoots::~ToxicBoots() {

		// 寿命前の削除でもParticleの発生を停止し、既存の粒子だけを寿命に従って消去
		MyParticle3d::Stop(particle_, MadoEngine::Particle::StopMode::Finish);
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void ToxicBoots::Initialize(InitializeDesc context) {

		// 再初期化時に前の設置地点のParticleを持ち越さないよう既存の再生を破棄
		MyParticle3d::Stop(particle_, MadoEngine::Particle::StopMode::Immediate);
		particle_ = {};
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		rotationYaw_ = 0.0f;
		groundNormal_ = { 0.0f, 1.0f, 0.0f };

		// Playerの移動Collider中心ではなく生成地点の地表へ固定
		transform_.translate = ownerPosition_ - Vector3(0.0f, 0.45f, 0.0f);
		constexpr float kMaxGroundSearchDistance = std::numeric_limits<float>::max();
		float groundSurfaceY = 0.0f;
		float surfaceY = 0.0f;
		bool foundGround = false;

		if (MyCollider::TryGetGroundSurfaceY(
			ownerPosition_, CollisionTag::MapBlock, surfaceY, kMaxGroundSearchDistance)) {
			groundSurfaceY = surfaceY;
			foundGround = true;
		}

		// 通常床と坂が重なる場所では生成地点に近い上側の地表を採用
		Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
		if (MyCollider::TryGetGroundSurface(
			ownerPosition_, CollisionTag::MapSlope, surfaceY, slopeNormal, kMaxGroundSearchDistance) &&
			(!foundGround || surfaceY > groundSurfaceY)) {
			groundSurfaceY = surfaceY;
			groundNormal_ = slopeNormal;
			foundGround = true;
		}

		if (foundGround) {
			transform_.translate.y = groundSurfaceY + kGroundOffset_ + MyRand::GetFloat(0.01f, 0.1f);
		}
		transform_.rotate = CreateGroundAlignedRotation(rotationYaw_, groundNormal_);
		transform_.scale = Vector3{ kBaseModelScale_, kBaseModelScale_, kBaseModelScale_ } * sizeRate_;

		// 専用Modelが用意されるまでEyeのModelとTextureを仮表示に使用
		model_ = MyModel::Create(objectName_, "Eye", SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("ToxicBootsTexture");
			model->SetTransform(transform_);
			model->SetCastShadow(false);
			model->SetReceiveShadow(false);
			model->SetLightingEnabled(false);
		}

		Sphere hitbox;
		hitbox.radius = kBaseAttackRadius_ * sizeRate_;
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		// 設置範囲内の複数Enemyへ接触判定を維持
		disappearsUponCollision_ = false;
		isReductionStarted_ = false;
		reductionTimer_.Reset();
		lifeTimer_.Start(lifeTime_, false);
		colorAnimationTimer_.Start(kColorCycleDuration, true);
		UpdateColor(0.0f);

		// 接地補正後のTranslateから発生させ、Modelの回転と拡縮はParticleへ適用しない設定
		MadoEngine::Particle::PlayDesc particleDesc;
		particleDesc.transform.translate = transform_.translate;
		particleDesc.sceneType = SceneType::Game;
		particleDesc.loopOverride = true;
		particle_ = MyParticle3d::Play("ToxicBootsParticle", particleDesc);
	}

	void ToxicBoots::Update(float deltaTime) {
		if (lifeTimer_.IsFinished()) {

			// 縮小演出の完了後にProjectileを破棄
			isDead_ = true;
			return;
		}

		if (lifeTimer_.GetProgress() >= kReductionStartRatio_ && !isReductionStarted_) {

			// Axeと同様に寿命末尾の三割を縮小演出へ割り当て
			isReductionStarted_ = true;
			reductionTimer_.Start(lifeTime_ * (1.0f - kReductionStartRatio_), false);

			// 収縮開始時点で新しい粒子の発生だけを止め、既存の粒子は自然に消去
			MyParticle3d::Stop(particle_, MadoEngine::Particle::StopMode::Finish);
		}

		// 生成位置を維持したまま接地面の法線を軸に回転
		rotationYaw_ += kRotationSpeed_ * deltaTime;
		transform_.rotate = CreateGroundAlignedRotation(rotationYaw_, groundNormal_);
		if (Model* model = MyModel::TryGet(model_)) {
			const float reductionProgress = [&]() {
				if (isReductionStarted_) {
					return (reductionTimer_.GetProgress());
				} else {
					return (0.0f);
				}
			}();
			const Vector3 baseScale = Vector3{ kBaseModelScale_, kBaseModelScale_, kBaseModelScale_ } * sizeRate_;

			// 消滅直前だけScaleを線形に縮小して突然の非表示を回避
			transform_.scale = Easing::Lerp(baseScale, Vector3{}, reductionProgress, EaseType::Linear);
			model->SetTransform(transform_);
		}

		// Modelの有無と切り離して発生位置を同期し、収縮中は停止済みのEffectを保持
		if (!isReductionStarted_ && particle_.HasValue()) {
			Transform3D particleTransform;
			particleTransform.translate = transform_.translate;
			MyParticle3d::SetTransform(particle_, particleTransform);
		}

		lifeTimer_.Update(deltaTime);
		reductionTimer_.Update(deltaTime);
		UpdateColor(deltaTime);

		MyDebugLine::AddShape(std::get<Sphere>(hitbox_));
	}

	void ToxicBoots::UpdateColor(float deltaTime) {
		float safeDeltaTime = 0.0f;
		if (std::isfinite(deltaTime)) {
			safeDeltaTime = std::max(deltaTime, 0.0f);
		}
		colorAnimationTimer_.Update(safeDeltaTime);

		// Eyeと同じCos波の往復補間で黄緑と緑の境界を滑らかに接続
		const float cycleRatio = colorAnimationTimer_.GetProgress();
		const float blendFactor = 0.5f - 0.5f * std::cos(
			cycleRatio * 2.0f * std::numbers::pi_v<float>
		);
		const Vector4 color = kColorStart + (kColorEnd - kColorStart) * blendFactor;

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetColor(color);
		}
	}
}
