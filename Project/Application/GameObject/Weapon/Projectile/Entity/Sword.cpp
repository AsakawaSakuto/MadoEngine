#include "Sword.h"
#include "Render/Object/3d/RibbonEffect/MyRibbonEffect3d.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Projectile {
	namespace {
		constexpr float kSweepHeightOffset = 0.5f;
		constexpr float kFullRotation = std::numbers::pi_v<float> * 2.0f;
		constexpr float kMaxCollisionAngleStep = std::numbers::pi_v<float> / 36.0f;
		constexpr float kBaseModelScale = 1.0f;
		constexpr float kMinSizeRate = 0.1f;
		constexpr float kMinSweepSpeed = 0.1f;
		constexpr float kMinDirectionLengthSq = 0.000001f;
		constexpr float kRotationEpsilon = 0.00001f;
		constexpr float kBladeCenterOffset = -3.0f;
		constexpr Vector3 kBladeHalfSize = { 0.5f, 0.5f, 2.0f };
		constexpr std::uint32_t kTrailVertexIndex = 2725;
	}

	Sword::~Sword() {

		// 本体消滅後は頂点の追跡を終了し、残った軌跡だけを寿命に従って消去
		MyRibbonEffect3d::Stop(trail_, MadoEngine::Ribbon::RibbonStopMode::Finish);
		if (!objectName_.empty()) {

			// Colliderの座標参照を本体の破棄前に解除
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Sword::Initialize(InitializeDesc context) {

		// 再初期化時に前の軌跡へ新しい剣の位置を接続しないよう既存Trailを破棄
		MyRibbonEffect3d::Stop(trail_, MadoEngine::Ribbon::RibbonStopMode::Immediate);
		trail_ = {};
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
		ownerUpDirection_ = context.ownerUpDirection;
		lastHitSweepIndices_.clear();
		sweepCount_ = std::max(context.shotMaxCount, 1);
		disappearsUponCollision_ = false;

		// 判定範囲の反転と回転時間のゼロ除算を防止
		if (!std::isfinite(sizeRate_)) {
			sizeRate_ = 1.0f;
		}
		sizeRate_ = std::max(sizeRate_, kMinSizeRate);
		if (!std::isfinite(moveSpeed_)) {
			moveSpeed_ = 12.0f;
		}
		const float modelScale = kBaseModelScale * sizeRate_;
		transform_.scale = { modelScale, modelScale, modelScale };

		// 周回面内のTarget方向へ変換し、剣先が-Zへ向く読み込み後のModel軸を180度補正
		Vector3 toTarget = targetPosition_ - ownerPosition_;
		toTarget.y = 0.0f;
		Vector3 forward = { 0.0f, 0.0f, 1.0f };
		if (toTarget.LengthSq() > kMinDirectionLengthSq) {
			forward = toTarget.Normalized();
		}
		const Matrix4x4 sweepPlaneRotation = Matrix::MakeAffine(
			{ 1.0f, 1.0f, 1.0f }, CreateGroundAlignedRotation(0.0f, ownerUpDirection_), {});
		const Vector3 localForward = Matrix::Transform(forward, Matrix::Transpose(sweepPlaneRotation));
		startYaw_ = std::atan2(localForward.x, localForward.z) + std::numbers::pi_v<float>;

		// 寿命設定による途中消滅を防ぐため、指定回数の連続回転を終了条件へ統一
		sweepTimer_.Start(CalculateSweepDuration(moveSpeed_, sweepCount_), false);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("SwordModelTexture");
		}

		hitbox_ = OBB{};
		UpdatePose();

		// 初期姿勢を反映した頂点から履歴を開始し、原点から剣先までの不要な軌跡を防止
		if (const Model* model = MyModel::TryGet(model_)) {
			MadoEngine::Ribbon::RibbonEffectPlayDesc trailDesc;
			if (model->TryGetVertexWorldPosition(kTrailVertexIndex, trailDesc.transform.translate)) {
				trailDesc.sceneType = SceneType::Game;
				trailDesc.loopOverride = true;
				trail_ = MyRibbonEffect3d::Play("SwordTrail", trailDesc);
			}
		}

		previousSweepYaw_ = sweepYaw_;
		previousPivotPosition_ = transform_.translate;
		previousUpDirection_ = ownerUpDirection_;
		MyCollider::RegisterCollider(objectName_, CollisionTag::PlayerProjectileHitBox, &hitbox_, &hitboxPosition_);
	}

	void Sword::Update(float deltaTime) {

		// Pause中と不正な時間入力では薙ぎ払いを進行させない制約
		if (isDead_ || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return;
		}

		// 最終角度までの軌道を一度衝突判定へ公開してから次Frameで破棄
		if (sweepTimer_.IsFinished()) {
			isDead_ = true;
			return;
		}

		previousSweepYaw_ = sweepYaw_;
		previousPivotPosition_ = transform_.translate;
		previousUpDirection_ = ownerUpDirection_;
		sweepTimer_.Update(deltaTime);
		UpdatePose();
	}

	float Sword::CalculateSweepDuration(float sweepSpeed, int sweepCount) {

		// Weapon側の待機時間とProjectile側の回転時間で同じ速度制約を使用
		if (!std::isfinite(sweepSpeed)) {
			sweepSpeed = 12.0f;
		}
		return kFullRotation * static_cast<float>(std::max(sweepCount, 1)) /
			std::max(sweepSpeed, kMinSweepSpeed);
	}

	void Sword::SynchronizeOwnerPosition(const Vector3& ownerPosition) {
		ownerPosition_ = ownerPosition;
	}

	void Sword::SynchronizeOwnerPose(const Vector3& ownerPosition, const Vector3& ownerUpDirection) {
		ownerPosition_ = ownerPosition;
		ownerUpDirection_ = ownerUpDirection;

		// 接地解決後の傾きと支点を描画と命中判定へ揃え、Timerの二重進行を回避
		UpdatePose();
		MyDebugLine::AddShape(hitbox_);
	}

	bool Sword::IsHitEnemy(const std::string& enemyColliderName) {
		if (isDead_) {
			return false;
		}
		if (MyCollider::IsHitName(enemyColliderName, colliderName_)) {
			collisionSweepIndex_ = GetSweepIndex(sweepYaw_);
			return true;
		}

		// 回転中に通過した角度もOBBで判定し、低FPSでも周囲のEnemyを取りこぼさない補間
		const float angleDifference = sweepYaw_ - previousSweepYaw_;
		const float tiltDifference = std::acos(std::clamp(Math::Dot(previousUpDirection_, ownerUpDirection_), -1.0f, 1.0f));
		const int segmentCount = std::max(
			1, static_cast<int>(std::ceil((std::abs(angleDifference) + tiltDifference) / kMaxCollisionAngleStep)));
		bool isHit = false;
		for (int segment = segmentCount - 1; segment >= 0; --segment) {
			const float progress = static_cast<float>(segment) / static_cast<float>(segmentCount);
			const float yaw = previousSweepYaw_ + angleDifference * progress;
			const Vector3 pivotPosition = Math::Lerp(previousPivotPosition_, transform_.translate, progress);

			// 坂への出入りで周回面が変化したFrameも、傾きの途中にある剣の軌道を補間
			const Vector3 upDirection = Math::Lerp(previousUpDirection_, ownerUpDirection_, progress).Normalized();
			UpdateHitbox(yaw, pivotPosition, upDirection);
			if (MyCollider::IsHitName(enemyColliderName, colliderName_)) {

				// 周回境界をまたぐFrameでも実際に接触した最新の周回へ命中を所属
				collisionSweepIndex_ = GetSweepIndex(yaw);
				isHit = true;
				break;
			}
		}

		// 一時的な補間Colliderを描画姿勢へ復元し、後続のEnemy判定へ途中の姿勢を残さない処理
		UpdateHitbox(sweepYaw_, transform_.translate, ownerUpDirection_);
		return isHit;
	}

	bool Sword::HandleEnemyCollision(std::uint32_t enemyId, const Vector3* bounceTargetPosition) {
		(void)bounceTargetPosition;

		// 接触中の連続Damageと回転終端での再接触を一回転単位で抑制
		if (isDead_) {
			return false;
		}
		const auto lastHit = lastHitSweepIndices_.find(enemyId);
		if (lastHit != lastHitSweepIndices_.end() && lastHit->second >= collisionSweepIndex_) {
			return false;
		}
		lastHitSweepIndices_[enemyId] = collisionSweepIndex_;
		return true;
	}

	void Sword::UpdatePose() {

		// Playerと共通の上方向へ支点を持ち上げ、斜面上でも周回面からの高さを維持
		transform_.translate = ownerPosition_ + ownerUpDirection_ * kSweepHeightOffset;
		const float sweepAngle = kFullRotation * static_cast<float>(sweepCount_) * sweepTimer_.GetProgress();

		// Euler角の折り返しから独立した連続角度で、補間軌道と周回ごとの命中制限を管理
		sweepYaw_ = startYaw_ + sweepAngle;
		collisionSweepIndex_ = GetSweepIndex(sweepYaw_);
		UpdateHitbox(sweepYaw_, transform_.translate, ownerUpDirection_);
		transform_.rotate = std::get<OBB>(hitbox_).rotate;
		moveDirection_ = -std::get<OBB>(hitbox_).orientation[2];
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);

			// Model更新を待たず現在姿勢の頂点をWorld座標へ変換し、剣の拡縮の二重適用を回避
			if (trail_.HasValue()) {
				Transform3D trailTransform;
				if (model->TryGetVertexWorldPosition(kTrailVertexIndex, trailTransform.translate)) {
					MyRibbonEffect3d::SetTransform(trail_, trailTransform);
				}
			}
		}
	}

	Vector3 Sword::CreateSweepRotation(float yaw, const Vector3& upDirection) const {

		// 描画とOBBに共通の回転順で周回面を構築し、ローカルY軸周りの等角速度を維持
		const Matrix4x4 rotation = Matrix::MakeRotateY(yaw) *
			Matrix::MakeAffine({ 1.0f, 1.0f, 1.0f }, CreateGroundAlignedRotation(0.0f, upDirection), {});
		Vector3 euler = {};
		const float sinY = std::clamp(-rotation.m[0][2], -1.0f, 1.0f);
		euler.y = std::asin(sinY);
		if (std::abs(std::cos(euler.y)) > kRotationEpsilon) {
			euler.x = std::atan2(rotation.m[1][2], rotation.m[2][2]);
			euler.z = std::atan2(rotation.m[0][1], rotation.m[0][0]);
			return euler;
		}

		// ジンバルロック付近ではZ回転を固定してModelとOBBに同じ姿勢を再現
		euler.x = std::atan2(rotation.m[1][0] * sinY, rotation.m[1][1]);
		euler.z = 0.0f;
		return euler;
	}

	int Sword::GetSweepIndex(float yaw) const {
		const float sweepProgress = std::max((yaw - startYaw_) / kFullRotation, 0.0f);

		// 最終角度を次の周回に含めず、回転完了時の余分な再命中を抑制
		return std::min(static_cast<int>(sweepProgress), sweepCount_ - 1);
	}

	void Sword::UpdateHitbox(float yaw, const Vector3& pivotPosition, const Vector3& upDirection) {
		OBB& hitbox = std::get<OBB>(hitbox_);
		hitbox.rotate = CreateSweepRotation(yaw, upDirection);
		hitbox.UpdateOrientation();
		const Vector3 halfSize = kBladeHalfSize * transform_.scale;

		// 読み込み後の剣が占めるローカルZ座標-5から-1の中央へColliderを配置
		hitboxPosition_ = pivotPosition + hitbox.orientation[2] * (kBladeCenterOffset * transform_.scale.z);
		hitbox.center = hitboxPosition_;
		hitbox.min = -halfSize;
		hitbox.max = halfSize;
	}
}
