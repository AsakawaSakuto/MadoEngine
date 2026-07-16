#include "EnemyMovement.h"
#include "Utility/Collider/MyCollider.h"
#include <algorithm>
#include <cmath>

namespace Enemy {

	namespace {
		constexpr float kDirectionEpsilon = 1e-5f;		  // 方向ベクトルの長さがこの値以下の場合は正規化を行わない
		constexpr float kRotationEpsilon = 1e-5f;		  // 回転計算でゼロ長ベクトルとして扱う閾値
		constexpr float kSlopeSnapDistance = 1.0f;		  // 坂の中心Y座標にスナップする距離の閾値
		constexpr float kSideClimbSpeed = 5.0f;			  // 側面上昇補助の速度
		constexpr float kMaxSideClimbHeight = 100.0f;	  // 側面上昇補助の最大上昇量
		constexpr float kBlockedProgressRate = 0.35f;	  // 側面で止められたと判定する前進量の割合
		constexpr float kSideClimbCrestGraceTime = 0.25f; // 側面上昇補助の頂点での猶予時間
		constexpr float kSideClimbCrestSpeedScale = 0.6f; // 側面上昇補助の頂点での速度倍率

		/// @brief 水平方向の長さの2乗を取得する
		/// @param value 対象のベクトル
		/// @return XZ平面での長さの2乗
		float GetHorizontalLengthSq(const Vector3& value) { return value.x * value.x + value.z * value.z; }

		/// @brief 長さがあれば正規化し、短すぎる場合は代替ベクトルを返す
		/// @param value 正規化するベクトル
		/// @param fallback 代替として使用するベクトル
		/// @return 正規化済み、または代替のベクトル
		Vector3 NormalizeOrFallback(const Vector3& value, const Vector3& fallback) {
			const float lengthSq = value.LengthSq();
			if (lengthSq < kRotationEpsilon) {
				return fallback;
			}

			const float invLength = 1.0f / std::sqrt(lengthSq);
			return value * invLength;
		}

		/// @brief 水平Yawから前方向ベクトルを作成する
		/// @param yaw 水平Yaw角度
		/// @return XZ平面上の前方向
		Vector3 CreateHorizontalForward(float yaw) { return { std::sin(yaw), 0.0f, std::cos(yaw) }; }

		/// @brief 水平Yawから右方向ベクトルを作成する
		/// @param yaw 水平Yaw角度
		/// @return XZ平面上の右方向
		Vector3 CreateHorizontalRight(float yaw) { return { std::cos(yaw), 0.0f, -std::sin(yaw) }; }

		/// @brief 回転行列の各軸からEuler角を復元する
		/// @param right ローカルX軸のワールド方向
		/// @param up ローカルY軸のワールド方向
		/// @param forward ローカルZ軸のワールド方向
		/// @return 復元したEuler角
		Vector3 ExtractEulerXYZ(const Vector3& right, const Vector3& up, const Vector3& forward) {
			Vector3 euler = {};
			const float sinY = std::clamp(-right.z, -1.0f, 1.0f);
			euler.y = std::asin(sinY);

			const float cosY = std::cos(euler.y);
			if (std::abs(cosY) > kRotationEpsilon) {
				euler.x = std::atan2(up.z, forward.z);
				euler.z = std::atan2(right.y, right.x);
				return euler;
			}

			euler.x = std::atan2(up.x * sinY, up.y);
			euler.z = 0.0f;
			return euler;
		}

		/// @brief 坂の法線と向きに沿ったModel回転を作成する
		/// @param faceYaw Enemyが向いている水平Yaw角度
		/// @param slopeNormal 坂上面の法線
		/// @return 坂に沿ったModel回転
		Vector3 CreateSlopeAlignedRotation(float faceYaw, const Vector3& slopeNormal) {
			const Vector3 up = NormalizeOrFallback(slopeNormal, { 0.0f, 1.0f, 0.0f });
			const Vector3 desiredForward = CreateHorizontalForward(faceYaw);

			Vector3 forward = desiredForward - up * Math::Dot(desiredForward, up);
			if (forward.LengthSq() < kRotationEpsilon) {
				const Vector3 horizontalRight = CreateHorizontalRight(faceYaw);
				forward = Math::Cross(horizontalRight, up);
			}
			forward = NormalizeOrFallback(forward, { 0.0f, 0.0f, 1.0f });

			Vector3 right = Math::Cross(up, forward);
			right = NormalizeOrFallback(right, CreateHorizontalRight(faceYaw));
			forward = NormalizeOrFallback(Math::Cross(right, up), forward);

			return ExtractEulerXYZ(right, up, forward);
		}
	} // namespace

	void Movement::Initialize() {
		velocityY_ = 0.0f;
		lastDeltaTime_ = 0.0f;
		sideClimbBaseY_ = 0.0f;
		sideClimbAmount_ = 0.0f;
		sideClimbCrestTimer_ = 0.0f;
		faceYaw_ = 0.0f;
		lastMoveStartPosition_ = {};
		lastDesiredHorizontalMove_ = {};
		currentGroundNormal_ = { 0.0f, 1.0f, 0.0f };
		isGrounded_ = false;
		isSideClimbing_ = false;
	}

	bool Movement::Update(float deltaTime, const Vector3& targetPosition, float moveSpeed, Transform3D& transform) {
		lastDeltaTime_ = std::max(0.0f, deltaTime);
		lastDesiredHorizontalMove_ = { 0.0f, 0.0f, 0.0f };
		lastMoveStartPosition_ = transform.translate;

		const Vector3 direction = GetDirectionToTarget(transform.translate, targetPosition);
		lastDesiredHorizontalMove_ = { direction.x * moveSpeed * lastDeltaTime_, 0.0f, direction.z * moveSpeed * lastDeltaTime_ };
		transform.translate.x += lastDesiredHorizontalMove_.x;
		transform.translate.z += lastDesiredHorizontalMove_.z;

		if (!isGrounded_) {
			velocityY_ -= gravity_ * lastDeltaTime_;
		}
		transform.translate.y += velocityY_ * lastDeltaTime_;

		transform.translate.x = std::clamp(transform.translate.x, mapLimit_.min.x, mapLimit_.max.x);
		transform.translate.y = std::clamp(transform.translate.y, mapLimit_.min.y, mapLimit_.max.y);
		transform.translate.z = std::clamp(transform.translate.z, mapLimit_.min.z, mapLimit_.max.z);

		if (GetHorizontalLengthSq(direction) > kDirectionEpsilon) {
			faceYaw_ = std::atan2(direction.x, direction.z);
			transform.rotate.y = faceYaw_;
		}

		return transform.translate.y > mapLimit_.min.y;
	}

	void Movement::ResolveAfterCollision(const std::string& movementColliderName, Transform3D& transform) {
		const bool isGroundContact = MyCollider::IsGroundContact(movementColliderName, CollisionTag::MapBlock);
		const bool isSlopeGroundContact = MyCollider::IsSlopeGroundContact(movementColliderName, CollisionTag::MapSlope);
		isGrounded_ = isGroundContact || isSlopeGroundContact;

		if (isGrounded_ && velocityY_ < 0.0f) {
			velocityY_ = 0.0f;
		}

		if (isSlopeGroundContact && velocityY_ <= 0.0f) {
			float slopeCenterY = 0.0f;
			if (MyCollider::TryGetSlopeGroundCenterY(movementColliderName, CollisionTag::MapSlope, slopeCenterY, kSlopeSnapDistance) &&
				transform.translate.y > slopeCenterY) {
				transform.translate.y = slopeCenterY;
			}
		}

		ApplySideClimbAssist(lastDeltaTime_, isGroundContact, isSlopeGroundContact, transform);
		UpdateGroundRotation(movementColliderName, isSlopeGroundContact, transform);
	}

	Vector3 Movement::GetDirectionToTarget(const Vector3& currentPosition, const Vector3& targetPosition) const {
		Vector3 direction = targetPosition - currentPosition;
		direction.y = 0.0f;

		const float lengthSq = GetHorizontalLengthSq(direction);
		if (lengthSq < kDirectionEpsilon) {
			return { 0.0f, 0.0f, 0.0f };
		}

		const float invLength = 1.0f / std::sqrt(lengthSq);
		return { direction.x * invLength, 0.0f, direction.z * invLength };
	}

	void Movement::ApplySideClimbAssist(float deltaTime, bool isGroundContact, bool isSlopeGroundContact, Transform3D& transform) {
		const bool isSideBlocked = IsSideBlockedThisFrame(transform.translate);
		const bool canStartClimb = isGroundContact || isSlopeGroundContact;
		const bool hasDesiredMove = GetHorizontalLengthSq(lastDesiredHorizontalMove_) > kDirectionEpsilon;
		const bool canContinueToCrest =
			isSideClimbing_ && !canStartClimb && hasDesiredMove && sideClimbCrestTimer_ < kSideClimbCrestGraceTime;

		if (!isSideBlocked && !canContinueToCrest) {
			ResetSideClimbAssist(transform.translate);
			return;
		}

		if (isSideBlocked && !canStartClimb && !isSideClimbing_) {
			ResetSideClimbAssist(transform.translate);
			return;
		}

		if (!isSideClimbing_) {
			isSideClimbing_ = true;
			sideClimbBaseY_ = transform.translate.y;
			sideClimbAmount_ = 0.0f;
		}

		if (isSideBlocked) {
			sideClimbCrestTimer_ = 0.0f;
		} else {
			sideClimbCrestTimer_ += deltaTime;
		}

		sideClimbAmount_ = std::max(sideClimbAmount_, transform.translate.y - sideClimbBaseY_);
		const float remainClimbHeight = kMaxSideClimbHeight - sideClimbAmount_;
		if (remainClimbHeight <= 0.0f) {
			velocityY_ = 0.0f;
			return;
		}

		const float climbSpeed = isSideBlocked ? kSideClimbSpeed : kSideClimbSpeed * kSideClimbCrestSpeedScale;
		const float climbStep = std::min(climbSpeed * deltaTime, remainClimbHeight);
		transform.translate.y += climbStep;
		sideClimbAmount_ += climbStep;
		velocityY_ = 0.0f;
		isGrounded_ = true;
	}

	void Movement::ResetSideClimbAssist(const Vector3& currentPosition) {
		isSideClimbing_ = false;
		sideClimbBaseY_ = currentPosition.y;
		sideClimbAmount_ = 0.0f;
		sideClimbCrestTimer_ = 0.0f;
	}

	bool Movement::IsSideBlockedThisFrame(const Vector3& currentPosition) const {
		const float desiredLengthSq = GetHorizontalLengthSq(lastDesiredHorizontalMove_);
		if (desiredLengthSq <= kDirectionEpsilon) {
			return false;
		}

		const float desiredLength = std::sqrt(desiredLengthSq);
		const Vector3 actualHorizontalMove = { currentPosition.x - lastMoveStartPosition_.x, 0.0f,
											   currentPosition.z - lastMoveStartPosition_.z };
		const float actualProgress =
			(actualHorizontalMove.x * lastDesiredHorizontalMove_.x + actualHorizontalMove.z * lastDesiredHorizontalMove_.z) / desiredLength;

		return actualProgress < desiredLength * kBlockedProgressRate;
	}

	void Movement::UpdateGroundRotation(const std::string& movementColliderName, bool isSlopeGroundContact, Transform3D& transform) {
		Vector3 targetGroundNormal = { 0.0f, 1.0f, 0.0f };
		Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
		if (isSlopeGroundContact && MyCollider::TryGetSlopeGroundNormal(movementColliderName, CollisionTag::MapSlope, slopeNormal)) {
			targetGroundNormal = NormalizeOrFallback(slopeNormal, { 0.0f, 1.0f, 0.0f });
		}

		const float normalT = std::clamp(1.0f - std::exp(-modelGroundNormalFollowSpeed_ * lastDeltaTime_), 0.0f, 1.0f);
		currentGroundNormal_ = NormalizeOrFallback(Math::Lerp(currentGroundNormal_, targetGroundNormal, normalT), targetGroundNormal);
		transform.rotate = CreateSlopeAlignedRotation(faceYaw_, currentGroundNormal_);
	}

} // namespace Enemy
