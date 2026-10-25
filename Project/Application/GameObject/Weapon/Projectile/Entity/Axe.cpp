#include "Axe.h"
#include "Render/Object/3d/RibbonEffect/MyRibbonEffect3d.h"
#include "../../../Map/MapLimit.h"
#include <cmath>

namespace Projectile {

	Axe::~Axe() {

		// 本体の破棄後は新しい軌跡の生成を停止し、残った部分だけを寿命に従って消去
		for (const auto& trail : trails_) {
			MyRibbonEffect3d::Stop(trail, MadoEngine::Ribbon::RibbonStopMode::Finish);
		}
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Axe::Initialize(InitializeDesc context) {

		// 再初期化時に前の軌跡へ新しい発射位置を接続しないよう既存Trailを破棄
		for (auto& trail : trails_) {
			MyRibbonEffect3d::Stop(trail, MadoEngine::Ribbon::RibbonStopMode::Immediate);
			trail = {};
		}
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("AxeModelTexture");
		}

		transform_.translate = ownerPosition_;
		transform_.scale = { 0.5f * sizeRate_, 0.5f * sizeRate_, 0.5f * sizeRate_ };

		SetMoveDirectionTowards(targetPosition_, true);

		AABB hitbox;
		float boxSize = 0.5f * sizeRate_;
		hitbox.min = { -boxSize, -boxSize, -boxSize };
		hitbox.max = { boxSize, boxSize, boxSize };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		isReductionStarted_ = false;
		disappearsUponCollision_ = false;

		// 一秒間の直進後に滞留寿命へ移行する二段階Timer
		reductionTimer_.Reset();
		startTimer_.Start(1.0f, false);

		// 各頂点から独立した履歴を開始し、二つの発生点を一本の軌跡で接続しない構成
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
			for (std::size_t trailIndex = 0; trailIndex < trails_.size(); ++trailIndex) {
				MadoEngine::Ribbon::RibbonEffectPlayDesc trailDesc;
				if (model->TryGetVertexWorldPosition(kTrailVertexIndices[trailIndex], trailDesc.transform.translate)) {
					trailDesc.sceneType = SceneType::Game;
					trailDesc.loopOverride = true;
					trails_[trailIndex] = MyRibbonEffect3d::Play("AxeTrail", trailDesc);
				}
			}
		}
	}

	void Axe::Update(float deltaTime) {

		// Map外への移動または滞留寿命の終了でProjectileを破棄
		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox) || lifeTimer_.IsFinished()) {
			isDead_ = true;
			return;
		}

		if (startTimer_.IsActive()) {

			// 初期移動期間だけ前方へ進み、終了後はその場で回転
			transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;
		}

		if (startTimer_.IsFinished()) {
			if (!lifeTimer_.IsActive()) {
				lifeTimer_.Start(lifeTime_, false);
			}
		}

		if (lifeTimer_.IsActive() && lifeTimer_.GetProgress() >= 0.7f && !isReductionStarted_) {

			// 寿命末尾の三割を縮小演出へ割り当て
			isReductionStarted_ = true;
			reductionTimer_.Start(lifeTime_ * 0.3f, false);
		}

		// 滞留中も武器らしい視認性を保つ一定速度の回転
		transform_.rotate.y += 3.14f * deltaTime;

		if (Model* model = MyModel::TryGet(model_)) {

			// 消滅直前だけScaleを連続的に縮小して突然の非表示を回避
			const float reductionProgress = [&]() {
				if (isReductionStarted_) {
					return (reductionTimer_.GetProgress());
				} else {
					return (0.0f);
				}
			}();
			const float scaleValue = 0.5f * sizeRate_ * (1.0f - reductionProgress);
			transform_.scale = Easing::Lerp(Vector3{ scaleValue,scaleValue,scaleValue }, Vector3{ 0.0f,0.0f,0.0f }, reductionProgress, EaseType::Linear);
			model->SetTransform(transform_);

			// 移動と回転、消滅前の縮小を反映した頂点のWorld座標へTrailを追従
			for (std::size_t trailIndex = 0; trailIndex < trails_.size(); ++trailIndex) {
				if (!trails_[trailIndex].HasValue()) {
					continue;
				}
				Transform3D trailTransform;
				if (model->TryGetVertexWorldPosition(kTrailVertexIndices[trailIndex], trailTransform.translate)) {
					MyRibbonEffect3d::SetTransform(trails_[trailIndex], trailTransform);
				}
			}
		}

		lifeTimer_.Update(deltaTime);
		startTimer_.Update(deltaTime);
		reductionTimer_.Update(deltaTime);

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}
}
