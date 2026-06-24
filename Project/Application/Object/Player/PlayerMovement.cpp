#include "PlayerMovement.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kRotationEpsilon = 1e-5f;

	/// @brief 長さがある場合は正規化し、短すぎる場合は代替ベクトルを返します。
	/// @param value 正規化するベクトルです。
	/// @param fallback 代替ベクトルです。
	/// @return 正規化済みベクトルです。
	Vector3 NormalizeOrFallback(const Vector3& value, const Vector3& fallback) {
		const float lengthSq = value.LengthSq();
		if (lengthSq < kRotationEpsilon) {
			return fallback;
		}

		const float invLength = 1.0f / std::sqrt(lengthSq);
		return value * invLength;
	}

	/// @brief 水平Yawから前方向ベクトルを作成します。
	/// @param yaw 水平Yaw角度です。
	/// @return 水平面上の前方向です。
	Vector3 CreateHorizontalForward(float yaw) {
		return { std::sin(yaw), 0.0f, std::cos(yaw) };
	}

	/// @brief 水平Yawから右方向ベクトルを作成します。
	/// @param yaw 水平Yaw角度です。
	/// @return 水平面上の右方向です。
	Vector3 CreateHorizontalRight(float yaw) {
		return { std::cos(yaw), 0.0f, -std::sin(yaw) };
	}

	/// @brief 回転行列の各軸からMakeAffineと同じ順序のEuler角を復元します。
	/// @param right ローカルX軸のワールド方向です。
	/// @param up ローカルY軸のワールド方向です。
	/// @param forward ローカルZ軸のワールド方向です。
	/// @return 復元したEuler角です。
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

	/// @brief 斜面法線と水平向きに沿ったModel回転を作成します。
	/// @param faceYaw Playerが向いている水平Yaw角度です。
	/// @param slopeNormal Slope上面の法線です。
	/// @return 斜面に沿ったModel回転です。
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
}

void PlayerMovement::Initialize() {
	currentMotion_ = PlayerMotion::Idle;
	currentGroundNormal_ = { 0.0f, 1.0f, 0.0f };
}

void PlayerMovement::Update(float deltaTime, Transform3D& transform, const Camera* camera, const PlayerMoveInput& input) {
	Move(deltaTime, transform, camera, input);
	Jump(deltaTime, transform, input);
	ApplySlopeGroundSnap(deltaTime, transform);
}

void PlayerMovement::SetGroundContact(bool isGroundContact, bool isSlopeGroundContact, const PlayerMoveInput& input) {
	if (isGroundContact || isSlopeGroundContact) {
		if (velocityY_ < 0.0f) {
			velocityY_ = 0.0f;
		}
		isGrounded_ = true;

		if (!input.isCrouching) {
			slideVelocity_ = { 0.0f, 0.0f, 0.0f };
		}
		if (velocityY_ <= 0.0f) {
			jumpMoveVelocity_ = { 0.0f, 0.0f, 0.0f };
		}
	} else {
		isGrounded_ = false;
	}
}

void PlayerMovement::Move(float deltaTime, Transform3D& transform, const Camera* camera, const PlayerMoveInput& input) {
	hasMoveInput_ = false;
	lastMoveDirection_ = { 0.0f, 0.0f, 0.0f };

	if (!camera) {
		return;
	}

	const bool isCrouching = input.isCrouching;
	const bool isCrouchingStarted = input.isCrouchingStarted;
	const Vector2 moveInput = input.move;
	const float inputLengthSq = moveInput.x * moveInput.x + moveInput.y * moveInput.y;
	bool hasMoveInput = inputLengthSq >= 1e-5f;

	const float yaw = camera->GetRotation().y;
	const Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };
	const Vector3 right = { std::cos(yaw), 0.0f, -std::sin(yaw) };

	Vector3 moveDir = {
		forward.x * moveInput.y + right.x * moveInput.x,
		0.0f,
		forward.z * moveInput.y + right.z * moveInput.x
	};

	const float moveLengthSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
	if (moveLengthSq > 1e-5f) {
		const float moveLength = std::sqrt(moveLengthSq);
		lastMoveDirection_ = { moveDir.x / moveLength, 0.0f, moveDir.z / moveLength };
		hasMoveInput_ = true;
		faceYaw_ = std::atan2(moveDir.x, moveDir.z);
		transform.rotate.y = faceYaw_;
	} else {
		hasMoveInput = false;
	}

	if (!isCrouching && hasMoveInput) {
		transform.translate.x += moveDir.x * movementParams_.moveSpeed_ * deltaTime;
		transform.translate.z += moveDir.z * movementParams_.moveSpeed_ * deltaTime;
		currentMotion_ = PlayerMotion::Walk;
	} else if (!isCrouching && !isGrounded_) {
		currentMotion_ = PlayerMotion::Jump;
	} else if (!isCrouching) {
		currentMotion_ = PlayerMotion::Idle;
	}

	if (!isCrouching) {
		ApplyJumpMoveBoost(deltaTime, transform);
	}

	UpdateSliding(deltaTime, transform, isCrouching, isCrouchingStarted, moveDir, hasMoveInput);
}

void PlayerMovement::UpdateSliding(float deltaTime, Transform3D& transform, bool isCrouching, bool isCrouchingStarted, const Vector3& moveDir, bool hasMoveInput) {
	if (isCrouching) {
		currentMotion_ = PlayerMotion::Crouching;
	}

	if (isCrouchingStarted && hasMoveInput) {
		slideVelocity_.x = moveDir.x * movementParams_.slideStartSpeed_;
		slideVelocity_.z = moveDir.z * movementParams_.slideStartSpeed_;
	}

	Vector3 slopeDownDirection = { 0.0f, 0.0f, 0.0f };
	const bool isCrouchingOnSlope = isCrouching && TryGetSlopeDownDirection(slopeDownDirection);
	if (isCrouchingOnSlope) {
		slideVelocity_.x += slopeDownDirection.x * movementParams_.slopeSlideAcceleration_ * deltaTime;
		slideVelocity_.z += slopeDownDirection.z * movementParams_.slopeSlideAcceleration_ * deltaTime;
	}

	const float slideSpeedSq = slideVelocity_.x * slideVelocity_.x + slideVelocity_.z * slideVelocity_.z;
	if (isCrouching && hasMoveInput && slideSpeedSq > 1e-6f) {
		const float slideSpeed = std::sqrt(slideSpeedSq);
		Vector3 currentDir = { slideVelocity_.x / slideSpeed, 0.0f, slideVelocity_.z / slideSpeed };
		const float steerT = std::clamp(movementParams_.slideSteerRate_ * deltaTime, 0.0f, 1.0f);
		Vector3 steeredDir = {
			currentDir.x + (moveDir.x - currentDir.x) * steerT,
			0.0f,
			currentDir.z + (moveDir.z - currentDir.z) * steerT
		};

		const float steeredLengthSq = steeredDir.x * steeredDir.x + steeredDir.z * steeredDir.z;
		if (steeredLengthSq > 1e-6f) {
			const float steeredLength = std::sqrt(steeredLengthSq);
			slideVelocity_.x = steeredDir.x / steeredLength * slideSpeed;
			slideVelocity_.z = steeredDir.z / steeredLength * slideSpeed;
		}
	}

	const float steeredSlideSpeedSq = slideVelocity_.x * slideVelocity_.x + slideVelocity_.z * slideVelocity_.z;
	if (steeredSlideSpeedSq > movementParams_.maxSlideSpeed_ * movementParams_.maxSlideSpeed_) {
		const float slideSpeed = std::sqrt(steeredSlideSpeedSq);
		const float speedScale = movementParams_.maxSlideSpeed_ / slideSpeed;
		slideVelocity_.x *= speedScale;
		slideVelocity_.z *= speedScale;
	}

	transform.translate.x += slideVelocity_.x * deltaTime;
	transform.translate.z += slideVelocity_.z * deltaTime;

	if (std::abs(slideVelocity_.x) > 0.001f || std::abs(slideVelocity_.z) > 0.001f) {
		faceYaw_ = std::atan2(slideVelocity_.x, slideVelocity_.z);
		transform.rotate.y = faceYaw_;
	}

	const float friction = isCrouching ? movementParams_.slideFriction_ : slideReleaseFriction_;
	ApplySlideFriction(deltaTime, isCrouchingOnSlope ? movementParams_.slideFriction_ * 0.35f : friction);
}

bool PlayerMovement::TryGetSlopeDownDirection(Vector3& outDownDirection) const {
	Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
	if (!MyCollider::IsSlopeGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope) ||
		!MyCollider::TryGetSlopeGroundNormal(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, slopeNormal)) {
		return false;
	}

	outDownDirection = { slopeNormal.x, 0.0f, slopeNormal.z };
	const float downLengthSq = outDownDirection.x * outDownDirection.x + outDownDirection.z * outDownDirection.z;
	if (downLengthSq < 1e-5f) {
		return false;
	}

	const float downLength = std::sqrt(downLengthSq);
	outDownDirection.x /= downLength;
	outDownDirection.z /= downLength;
	return true;
}

void PlayerMovement::ApplySlideFriction(float deltaTime, float friction) {
	const float slideSpeedSq = slideVelocity_.x * slideVelocity_.x + slideVelocity_.z * slideVelocity_.z;
	if (slideSpeedSq < 1e-6f) {
		slideVelocity_.x = 0.0f;
		slideVelocity_.z = 0.0f;
		return;
	}

	const float slideSpeed = std::sqrt(slideSpeedSq);
	const float nextSpeed = std::max(0.0f, slideSpeed - friction * deltaTime);
	if (nextSpeed <= 0.001f) {
		slideVelocity_.x = 0.0f;
		slideVelocity_.z = 0.0f;
		return;
	}

	const float speedScale = nextSpeed / slideSpeed;
	slideVelocity_.x *= speedScale;
	slideVelocity_.z *= speedScale;
}

void PlayerMovement::ApplyJumpMoveBoost(float deltaTime, Transform3D& transform) {
	if (isGrounded_ && velocityY_ <= 0.0f) {
		jumpMoveVelocity_ = { 0.0f, 0.0f, 0.0f };
		return;
	}

	const float boostSpeedSq = jumpMoveVelocity_.x * jumpMoveVelocity_.x + jumpMoveVelocity_.z * jumpMoveVelocity_.z;
	if (boostSpeedSq < 1e-6f) {
		jumpMoveVelocity_.x = 0.0f;
		jumpMoveVelocity_.z = 0.0f;
		return;
	}

	transform.translate.x += jumpMoveVelocity_.x * deltaTime;
	transform.translate.z += jumpMoveVelocity_.z * deltaTime;

	const float boostSpeed = std::sqrt(boostSpeedSq);
	const float nextSpeed = std::max(0.0f, boostSpeed - jumpMoveBoostFriction_ * deltaTime);
	if (nextSpeed <= 0.001f) {
		jumpMoveVelocity_.x = 0.0f;
		jumpMoveVelocity_.z = 0.0f;
		return;
	}

	const float speedScale = nextSpeed / boostSpeed;
	jumpMoveVelocity_.x *= speedScale;
	jumpMoveVelocity_.z *= speedScale;
}

void PlayerMovement::AddJumpMoveBoost(const PlayerMoveInput& input) {
	if (!hasMoveInput_ || input.isCrouching) {
		return;
	}

	jumpMoveVelocity_.x += lastMoveDirection_.x * movementParams_.jumpMoveBoostSpeed_;
	jumpMoveVelocity_.z += lastMoveDirection_.z * movementParams_.jumpMoveBoostSpeed_;

	const float maxBoostSpeed = movementParams_.jumpMoveBoostSpeed_ * 2.0f;
	const float boostSpeedSq = jumpMoveVelocity_.x * jumpMoveVelocity_.x + jumpMoveVelocity_.z * jumpMoveVelocity_.z;
	if (boostSpeedSq <= maxBoostSpeed * maxBoostSpeed) {
		return;
	}

	const float boostSpeed = std::sqrt(boostSpeedSq);
	const float speedScale = maxBoostSpeed / boostSpeed;
	jumpMoveVelocity_.x *= speedScale;
	jumpMoveVelocity_.z *= speedScale;
}

void PlayerMovement::ApplySlopeGroundSnap(float deltaTime, Transform3D& transform) {
	if (!isGrounded_ || velocityY_ > 0.0f) {
		return;
	}

	float slopeCenterY = 0.0f;
	float snapDistance = slopeSnapDistance_ + movementParams_.gravity_ * deltaTime * deltaTime;
	if (!MyCollider::TryGetSlopeGroundCenterY(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, slopeCenterY, snapDistance)) {
		return;
	}

	if (transform.translate.y > slopeCenterY) {
		transform.translate.y = slopeCenterY;
		if (velocityY_ < 0.0f) {
			velocityY_ = 0.0f;
		}
		isGrounded_ = true;
	}
}

void PlayerMovement::UpdateModelTransform(float deltaTime, Transform3D& transform, Model* model, bool isSlopeGroundContact) {
	if (!model) {
		return;
	}

	Vector3 targetGroundNormal = { 0.0f, 1.0f, 0.0f };
	Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
	if (isSlopeGroundContact && MyCollider::TryGetSlopeGroundNormal(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, slopeNormal)) {
		targetGroundNormal = NormalizeOrFallback(slopeNormal, { 0.0f, 1.0f, 0.0f });
	}

	const float normalT = std::clamp(1.0f - std::exp(-modelGroundNormalFollowSpeed_ * deltaTime), 0.0f, 1.0f);
	currentGroundNormal_ = NormalizeOrFallback(Math::Lerp(currentGroundNormal_, targetGroundNormal, normalT), targetGroundNormal);
	transform.rotate = CreateSlopeAlignedRotation(faceYaw_, currentGroundNormal_);

	model->SetPosition(transform.translate);
	model->SetRotation(transform.rotate);
}

void PlayerMovement::Jump(float deltaTime, Transform3D& transform, const PlayerMoveInput& input) {
	if (isGrounded_) {
		remainingJumpCount_ = movementParams_.jumpCount_;
	}

	if (remainingJumpCount_ > 0 && input.isJumpTriggered) {
		velocityY_ = movementParams_.jumpPower_;
		isGrounded_ = false;
		AddJumpMoveBoost(input);
		remainingJumpCount_--;
		Logger::Output("[Engine] ジャンプ開始 残り回数: " + std::to_string(remainingJumpCount_), Logger::Level::Application);
	}

	if (!isGrounded_) {
		velocityY_ -= movementParams_.gravity_ * deltaTime;
	}

	transform.translate.y += velocityY_ * deltaTime;

	if (transform.translate.y <= groundY_) {
		transform.translate.y = groundY_;
		velocityY_ = 0.0f;
		if (!isGrounded_) {
			isGrounded_ = true;
			remainingJumpCount_ = movementParams_.jumpCount_;
		}
	}
}
