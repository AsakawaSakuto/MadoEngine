#include "Sword.h"
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
		constexpr float kBladeCenterOffset = -3.0f;
		constexpr Vector3 kBladeHalfSize = { 0.5f, 0.5f, 2.0f };
	}

	Sword::~Sword() {
		if (!objectName_.empty()) {

			// Colliderの座標参照を本体の破棄前に解除
			MyCollider::RemoveCollider(objectName_);
			MyModel::RequestDestroy(model_);
		}
	}

	void Sword::Initialize(InitializeDesc context) {
		objectName_ = context.projectileName + "_" + std::to_string(context.projectileId);
		InitializeCommonProperties(context, objectName_);
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

		// Assimpの左手系変換で剣先が-Zへ向くため、Target方向へ開始角度を180度補正
		Vector3 toTarget = targetPosition_ - ownerPosition_;
		toTarget.y = 0.0f;
		Vector3 forward = { 0.0f, 0.0f, 1.0f };
		if (toTarget.LengthSq() > kMinDirectionLengthSq) {
			forward = toTarget.Normalized();
		}
		startYaw_ = std::atan2(forward.x, forward.z) + std::numbers::pi_v<float>;

		// 寿命設定による途中消滅を防ぐため、指定回数の連続回転を終了条件へ統一
		sweepTimer_.Start(CalculateSweepDuration(moveSpeed_, sweepCount_), false);

		model_ = MyModel::Create(objectName_, context.projectileName, SceneType::Game);
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTexture("SwordModelTexture");
		}

		hitbox_ = OBB{};
		UpdatePose();
		previousYaw_ = transform_.rotate.y;
		previousPivotPosition_ = transform_.translate;
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

		previousYaw_ = transform_.rotate.y;
		previousPivotPosition_ = transform_.translate;
		sweepTimer_.Update(deltaTime);
		UpdatePose();
		MyDebugLine::AddShape(hitbox_);
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

	bool Sword::IsHitEnemy(const std::string& enemyColliderName) {
		if (isDead_) {
			return false;
		}
		if (MyCollider::IsHitName(enemyColliderName, colliderName_)) {
			collisionSweepIndex_ = GetSweepIndex(transform_.rotate.y);
			return true;
		}

		// 回転中に通過した角度もOBBで判定し、低FPSでも周囲のEnemyを取りこぼさない補間
		const float currentYaw = transform_.rotate.y;
		const float angleDifference = currentYaw - previousYaw_;
		const int segmentCount = std::max(
			1, static_cast<int>(std::ceil(std::abs(angleDifference) / kMaxCollisionAngleStep)));
		bool isHit = false;
		for (int segment = segmentCount - 1; segment >= 0; --segment) {
			const float progress = static_cast<float>(segment) / static_cast<float>(segmentCount);
			const float yaw = previousYaw_ + angleDifference * progress;
			const Vector3 pivotPosition = Math::Lerp(previousPivotPosition_, transform_.translate, progress);
			UpdateHitbox(yaw, pivotPosition);
			if (MyCollider::IsHitName(enemyColliderName, colliderName_)) {

				// 周回境界をまたぐFrameでも実際に接触した最新の周回へ命中を所属
				collisionSweepIndex_ = GetSweepIndex(yaw);
				isHit = true;
				break;
			}
		}

		// 一時的な補間Colliderを描画姿勢へ復元し、後続のEnemy判定へ途中の姿勢を残さない処理
		UpdateHitbox(currentYaw, transform_.translate);
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

		// XZ平面に回転を固定し、Playerの移動に対して支点と高さを維持
		transform_.translate = ownerPosition_ + Vector3{ 0.0f, kSweepHeightOffset, 0.0f };
		const float sweepAngle = kFullRotation * static_cast<float>(sweepCount_) * sweepTimer_.GetProgress();
		transform_.rotate = { 0.0f, startYaw_ + sweepAngle, 0.0f };
		collisionSweepIndex_ = GetSweepIndex(transform_.rotate.y);
		UpdateHitbox(transform_.rotate.y, transform_.translate);
		moveDirection_ = -std::get<OBB>(hitbox_).orientation[2];
		if (Model* model = MyModel::TryGet(model_)) {
			model->SetTransform(transform_);
		}
	}

	int Sword::GetSweepIndex(float yaw) const {
		const float sweepProgress = std::max((yaw - startYaw_) / kFullRotation, 0.0f);

		// 最終角度を次の周回に含めず、回転完了時の余分な再命中を抑制
		return std::min(static_cast<int>(sweepProgress), sweepCount_ - 1);
	}

	void Sword::UpdateHitbox(float yaw, const Vector3& pivotPosition) {
		OBB& hitbox = std::get<OBB>(hitbox_);
		hitbox.rotate = { 0.0f, yaw, 0.0f };
		hitbox.UpdateOrientation();
		const Vector3 halfSize = kBladeHalfSize * transform_.scale;

		// 読み込み後の剣が占めるローカルZ座標-5から-1の中央へColliderを配置
		hitboxPosition_ = pivotPosition + hitbox.orientation[2] * (kBladeCenterOffset * transform_.scale.z);
		hitbox.center = hitboxPosition_;
		hitbox.min = -halfSize;
		hitbox.max = halfSize;
	}
}
