#include "Pistol.h"
#include "Render/Object/3d/RibbonEffect/MyRibbonEffect3d.h"
#include "../../../Map/MapLimit.h"
#include <cmath>

namespace Projectile {
	namespace {
		constexpr std::uint32_t kTrailVertexIndex = 382;
	}

	Pistol::~Pistol() {

		// 本体消滅後は新しい軌跡の生成を停止し、残った部分だけを寿命に従って消去
		MyRibbonEffect3d::Stop(trail_, MadoEngine::Ribbon::RibbonStopMode::Finish);
		if (!objectName_.empty()) {
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}
	
	void Pistol::Initialize(InitializeDesc context) {

		// 再初期化時に前の軌跡へ新しい発射位置を接続しないよう既存Trailを破棄
		MyRibbonEffect3d::Stop(trail_, MadoEngine::Ribbon::RibbonStopMode::Immediate);
		trail_ = {};
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("PistolTexture");
		}

		transform_.translate = ownerPosition_;
		transform_.scale = { 0.5f, 0.5f, 0.5f };

		SetMoveDirectionTowards(targetPosition_);

		AABB hitbox;
		hitbox.min = { -0.5f, -0.5f, -0.5f };
		hitbox.max = { 0.5f, 0.5f, 0.5f };
		hitbox_ = hitbox;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &transform_.translate);

		// 発射姿勢の頂点から履歴を開始し、Model原点からの不要な軌跡を防止
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
			MadoEngine::Ribbon::RibbonEffectPlayDesc trailDesc;
			if (model->TryGetVertexWorldPosition(kTrailVertexIndex, trailDesc.transform.translate)) {
				trailDesc.sceneType = SceneType::Game;
				trailDesc.loopOverride = true;
				trail_ = MyRibbonEffect3d::Play("PistolTrail", trailDesc);
			}
		}
	}

	void Pistol::Update(float deltaTime) {

		transform_.translate += moveDirection_ * moveSpeed_ * deltaTime;

		// MapLimitの包含領域を出たProjectileを即時失効
		if (!MyCollider::IsHitWithTag(objectName_, CollisionTag::MapLimitBox)) {
			isDead_ = true;
			return;
		}

		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);

			// 移動と回転、拡縮を反映した頂点のWorld座標へTrailを追従
			if (trail_.HasValue()) {
				Transform3D trailTransform;
				if (model->TryGetVertexWorldPosition(kTrailVertexIndex, trailTransform.translate)) {
					MyRibbonEffect3d::SetTransform(trail_, trailTransform);
				}
			}
		}

		MyDebugLine::AddShape(std::get<AABB>(hitbox_));
	}

}
