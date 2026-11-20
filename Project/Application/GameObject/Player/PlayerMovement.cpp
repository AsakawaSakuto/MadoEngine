#include "PlayerMovement.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
	constexpr float kRotationEpsilon = 1e-5f;
	constexpr float kBlockedProgressRate = 0.35f;
	constexpr float kBunnyHopTimeEpsilon = 1e-6f;
	constexpr float kKnockbackDamping = 10.0f;
	constexpr float kKnockbackVelocityEpsilonSq = 0.000001f;
	constexpr float kMaxKnockbackSpeed = 30.0f;

	/// @brief 長さがある場合は正規化し、短すぎる場合は代替ベクトルを返却
	/// @param value 正規化するベクトル
	/// @param fallback 代替ベクトル
	/// @return 正規化済みベクトル
	Vector3 NormalizeOrFallback(const Vector3& value, const Vector3& fallback) {
		const float lengthSq = value.LengthSq();
		if (lengthSq < kRotationEpsilon) {
			return fallback;
		}

		const float invLength = 1.0f / std::sqrt(lengthSq);
		return value * invLength;
	}

	/// @brief 水平Yawから前方向ベクトルを作成
	/// @param yaw 水平Yaw角度
	/// @return 水平面上の前方向
	Vector3 CreateHorizontalForward(float yaw) {
		return { std::sin(yaw), 0.0f, std::cos(yaw) };
	}

	/// @brief 水平Yawから右方向ベクトルを作成
	/// @param yaw 水平Yaw角度
	/// @return 水平面上の右方向
	Vector3 CreateHorizontalRight(float yaw) {
		return { std::cos(yaw), 0.0f, -std::sin(yaw) };
	}

	/// @brief 回転行列の各軸からMakeAffineと同じ順序のEuler角を復元
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

		// ジンバルロック付近ではZ回転を固定して不定な解を回避
		euler.x = std::atan2(up.x * sinY, up.y);
		euler.z = 0.0f;
		return euler;
	}

	/// @brief 斜面法線と水平向きに沿ったModel回転を作成
	/// @param faceYaw Playerが向いている水平Yaw角度
	/// @param slopeNormal Slope上面の法線
	/// @return 斜面に沿ったModel回転
	Vector3 CreateSlopeAlignedRotation(float faceYaw, const Vector3& slopeNormal) {
		const Vector3 up = NormalizeOrFallback(slopeNormal, { 0.0f, 1.0f, 0.0f });
		const Vector3 desiredForward = CreateHorizontalForward(faceYaw);

		// 水平方向を斜面へ射影してModelの前方向を斜面上に拘束
		Vector3 forward = desiredForward - up * Math::Dot(desiredForward, up);
		if (forward.LengthSq() < kRotationEpsilon) {

			// 前方向と法線が平行に近い場合は右方向から安定した前方向を再構築
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

namespace Player {

	void Movement::RegisterDebugParameters(MadoEngine::ParameterStore& parameterStore) {
		debugParameter_ = std::make_unique<MadoEngine::DebugParameter>(parameterStore, "Player");
		MadoEngine::ParameterAttributes attributes;
		attributes.minimum = 0.0;
		attributes.maximum = 100.0;

		// 初期反映を移動状態の初期化より先に実施し、保存済みジャンプ上限を使用
		debugParameter_->Register("移動速度", movementParams_.moveSpeed_, 0, "移動", attributes);
		debugParameter_->Register("ジャンプ力", movementParams_.jumpPower_, 1, "移動", attributes);
		debugParameter_->Register("重力", movementParams_.gravity_, 2, "移動", attributes);
		debugParameter_->Register("スライド開始速度", movementParams_.slideStartSpeed_, 0, "スライド", attributes);
		debugParameter_->Register("スライド方向補正率", movementParams_.slideSteerRate_, 1, "スライド", attributes);
		debugParameter_->Register("斜面スライド加速度", movementParams_.slopeSlideAcceleration_, 2, "スライド", attributes);
		debugParameter_->Register("最大スライド速度", movementParams_.maxSlideSpeed_, 3, "スライド", attributes);
		debugParameter_->Register("スライド摩擦", movementParams_.slideFriction_, 4, "スライド", attributes);
		debugParameter_->Register("ジャンプ横初速", movementParams_.jumpMoveBoostSpeed_, 3, "移動", attributes);
		debugParameter_->Register("加速量", movementParams_.bunnyHopSpeedBoost_, 1, "バニーホップ", attributes);
		debugParameter_->Register("最大水平速度", movementParams_.bunnyHopMaxSpeed_, 2, "バニーホップ", attributes);
		debugParameter_->Register("地上減速量", movementParams_.bunnyHopGroundFriction_, 3, "バニーホップ", attributes);
		debugParameter_->Register("方向補正率", movementParams_.bunnyHopSteerRate_, 4, "バニーホップ", attributes);
		attributes.maximum = 1.0;
		attributes.dragSpeed = 0.01f;
		debugParameter_->Register("受付時間", movementParams_.bunnyHopWindow_, 0, "バニーホップ", attributes);
		attributes.maximum = 100.0;
		attributes.dragSpeed = 0.1f;
		debugParameter_->Register("壁上り速度", movementParams_.wallClimbSpeed_, 0, "壁上り", attributes);
		attributes.maximum = 60.0;
		debugParameter_->Register("壁上り最大時間", movementParams_.wallClimbMaxDuration_, 1, "壁上り", attributes);
		attributes.maximum = 1000.0;
		attributes.dragSpeed = 1.0f;
		debugParameter_->Register("ジャンプ回数", movementParams_.jumpCount_, 4, "移動", attributes);

		// ゲーム挙動へ影響させず選択と永続化を確認するため専用値を登録
		debugParameter_->RegisterEaseType("イージング", debugEaseType_, 0, "確認用");
		parameterStore.SetGroupPriority("Player/移動", 0);
		parameterStore.SetGroupPriority("Player/スライド", 1);
		parameterStore.SetGroupPriority("Player/壁上り", 2);
		parameterStore.SetGroupPriority("Player/バニーホップ", 3);
		parameterStore.SetGroupPriority("Player/確認用", 4);
		debugParameter_->Apply();
	}

	void Movement::Initialize() {
		currentMotion_ = Player::Motion::Idle;
		currentGroundNormal_ = { 0.0f, 1.0f, 0.0f };
		jumpStartedThisFrame_ = false;
		landedThisFrame_ = false;

		// 生成直後の重力落下で地形内部へ入らないよう初期状態を接地へ固定
		velocityY_ = 0.0f;
		isGrounded_ = true;
		remainingJumpCount_ = GetMaxJumpCount();
		slideVelocity_ = {};
		jumpMoveVelocity_ = {};
		lastMoveStartPosition_ = {};
		lastAttemptedHorizontalMove_ = {};
		knockbackVelocity_ = {};
		hasWallClimbInput_ = false;
		wasHorizontalMoveBlocked_ = false;
		isWallClimbing_ = false;
		hasWallClimbStartedSinceLanding_ = false;
		wasGroundContact_ = true;
		isBaseGroundContact_ = false;
		hasBunnyHopLanding_ = false;
		isBunnyHopMomentumActive_ = false;
		timeSinceLanding_ = 0.0f;
		wallClimbElapsedTime_ = 0.0f;
	}

	void Movement::SetMoveSpeedMultiplier(float multiplier) {
		if (std::isfinite(multiplier)) {
			moveSpeedMultiplier_ = std::max(multiplier, 0.0f);
		} else {
			moveSpeedMultiplier_ = 1.0f;
		}
	}

	void Movement::SetAdditionalJumpCount(int additionalJumpCount) {
		const float previousMaxJumpCount = GetMaxJumpCount();
		additionalJumpCount_ = std::max(additionalJumpCount, 0);
		const float nextMaxJumpCount = GetMaxJumpCount();

		// 空中で補正が変わっても使用済み回数を維持し、増減した上限分だけ残り回数へ反映
		remainingJumpCount_ = std::clamp(
			remainingJumpCount_ + nextMaxJumpCount - previousMaxJumpCount,
			0.0f,
			nextMaxJumpCount
		);
	}

	void Movement::Update(float deltaTime, Transform3D& transform, const Camera* camera, const MoveInput& input) {
		if (debugParameter_) {

			// 編集された設定だけを反映し、速度などの実行状態は維持
			debugParameter_->ApplyIfDirty();
		}
		jumpStartedThisFrame_ = false;
		landedThisFrame_ = false;
		lastMoveStartPosition_ = transform.translate;
		lastAttemptedHorizontalMove_ = {};
		hasWallClimbInput_ = false;
		isBaseGroundContact_ = false;
		UpdateBunnyHopState(deltaTime);

		// 入力による向き変更や壁登り解除より先に、登っていた姿勢の背面へ離脱
		if (isWallClimbing_ && input.isJumpTriggered) {
			StartWallJump();
			ApplyJumpMoveBoost(deltaTime, transform);
		} else {
			Move(deltaTime, transform, camera, input);
			hasWallClimbInput_ = hasMoveInput_ && !input.isCrouching;
			if (!hasWallClimbInput_) {
				isWallClimbing_ = false;
			}
		}

		// 離脱初速または入力移動へ重力と外力を重ねてから接地中の斜面追従で位置を補正
		Jump(deltaTime, transform, input);
		UpdateKnockback(deltaTime, transform);
		ApplySlopeGroundSnap(deltaTime, transform);

		// Collider解決後の実移動量と比較するため解決前の水平移動量を保持
		lastAttemptedHorizontalMove_ = {
			transform.translate.x - lastMoveStartPosition_.x,
			0.0f,
			transform.translate.z - lastMoveStartPosition_.z
		};
	}

	void Movement::ApplyKnockback(const Vector3& direction, float power) {
		if (!std::isfinite(power) || power <= 0.0f ||
			!std::isfinite(direction.x) || !std::isfinite(direction.z)) {
			return;
		}

		const Vector3 horizontalDirection = { direction.x, 0.0f, direction.z };
		const float lengthSq = horizontalDirection.x * horizontalDirection.x + horizontalDirection.z * horizontalDirection.z;
		if (lengthSq <= kRotationEpsilon) {
			return;
		}

		// 指数減衰後の総移動距離がpowerと一致する初速へ変換
		const float velocityScale = power * kKnockbackDamping / std::sqrt(lengthSq);
		knockbackVelocity_ += horizontalDirection * velocityScale;

		const float speedSq = knockbackVelocity_.x * knockbackVelocity_.x + knockbackVelocity_.z * knockbackVelocity_.z;
		if (speedSq <= kMaxKnockbackSpeed * kMaxKnockbackSpeed) {
			return;
		}

		// 多数のEnemyから同時に受けても極端な移動量にならないよう合成速度を制限
		const float speedScale = kMaxKnockbackSpeed / std::sqrt(speedSq);
		knockbackVelocity_ *= speedScale;
	}

	void Movement::CaptureCollisionResult(const Vector3& resolvedPosition) {

		// SceneManagerが前フレーム末尾に押し戻した座標を、
		// 次の移動情報で上書きする前に前フレームの希望移動量と比較
		wasHorizontalMoveBlocked_ = IsHorizontalMoveBlocked(resolvedPosition);
	}

	void Movement::SetGroundContact(bool isGroundContact, bool isSlopeGroundContact, const MoveInput& input) {

		// 上昇中の接触を着地とみなさず、Colliderがない基準地面も同じ着地判定へ統合
		const bool hasGroundContact = (isGroundContact || isSlopeGroundContact || isBaseGroundContact_) && velocityY_ <= 0.0f;
		if (hasGroundContact && !wasGroundContact_) {

			// 空中または壁登り状態から地表へ着地した時点で利用可能時間と開始履歴を初期化
			landedThisFrame_ = true;
			wallClimbElapsedTime_ = 0.0f;
			hasWallClimbStartedSinceLanding_ = false;
			timeSinceLanding_ = 0.0f;
			hasBunnyHopLanding_ = true;
			if (input.move.LengthSq() <= 1e-5f) {

				// 移動入力のない着地では意図しない滑走を防ぐため、保持中の水平速度をすべて破棄
				jumpMoveVelocity_ = {};
				slideVelocity_ = {};
				knockbackVelocity_ = {};
				isBunnyHopMomentumActive_ = false;
			}
		}

		if (hasGroundContact) {

			// 上昇中のジャンプ速度を維持するため落下中の速度だけ接地時に停止
			if (velocityY_ < 0.0f) {
				velocityY_ = 0.0f;
			}
			isGrounded_ = true;

			if (!input.isCrouching) {
				slideVelocity_ = { 0.0f, 0.0f, 0.0f };
			}
		} else {
			isGrounded_ = false;
			hasBunnyHopLanding_ = false;
		}

		wasGroundContact_ = hasGroundContact;
	}

	float Movement::GetWallClimbRemainingTime() const {
		const float maxDuration = GetWallClimbMaxDuration();
		return std::max(0.0f, maxDuration - wallClimbElapsedTime_);
	}

	float Movement::GetWallClimbMaxDuration() const {
		return std::max(0.0f, movementParams_.wallClimbMaxDuration_);
	}

	void Movement::UpdateWallClimb(float deltaTime, const MoveInput& input, Transform3D& transform) {
		const bool wasWallClimbing = isWallClimbing_;
		const bool isBlocked = wasHorizontalMoveBlocked_;
		if (input.isCrouching || jumpStartedThisFrame_ || !hasWallClimbInput_ || !isBlocked) {
			isWallClimbing_ = false;
			return;
		}

		const float maxDuration = std::max(0.0f, movementParams_.wallClimbMaxDuration_);
		if (wallClimbElapsedTime_ >= maxDuration) {
			isWallClimbing_ = false;
			if (wasWallClimbing) {
				StartWallClimbTimeoutJump();
			}
			return;
		}

		// 壁を離れた後も空中でゲージを表示できるよう着地まで開始履歴を維持
		hasWallClimbStartedSinceLanding_ = true;

		const float climbDeltaTime = std::min(
			std::max(0.0f, deltaTime),
			maxDuration - wallClimbElapsedTime_
		);

		// 上限までの残り時間だけ上昇へ反映してフレーム時間による超過を防止
		transform.translate.y += std::max(0.0f, movementParams_.wallClimbSpeed_) * climbDeltaTime;
		wallClimbElapsedTime_ += climbDeltaTime;
		velocityY_ = 0.0f;
		isWallClimbing_ = wallClimbElapsedTime_ < maxDuration;
		if (!isWallClimbing_) {
			StartWallClimbTimeoutJump();
			return;
		}

		currentMotion_ = Player::Motion::Climbing;
	}

	void Movement::Move(float deltaTime, Transform3D& transform, const Camera* camera, const MoveInput& input) {
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

		// Cameraの向きを基準に入力をワールド空間の水平移動へ変換
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

		const Vector3 moveVelocity = moveDir * (movementParams_.moveSpeed_ * moveSpeedMultiplier_);
		if (!isCrouching) {

			// 保持中の勢いを入力方向へ旋回させてから、歩行を含む合算速度を上限内へ制限
			UpdateBunnyHopSteering(deltaTime, moveDir);
			ClampBunnyHopSpeed(moveVelocity);
		}

		if (!isCrouching && hasMoveInput) {
			transform.translate.x += moveVelocity.x * deltaTime;
			transform.translate.z += moveVelocity.z * deltaTime;
			currentMotion_ = Player::Motion::Walk;
		} else if (!isCrouching && !isGrounded_) {
			currentMotion_ = Player::Motion::Jump;
		} else if (!isCrouching) {
			currentMotion_ = Player::Motion::Idle;
		}

		if (!isCrouching) {
			ApplyJumpMoveBoost(deltaTime, transform);
		}

		UpdateSliding(deltaTime, transform, isCrouching, isCrouchingStarted, moveDir, hasMoveInput);
	}

	void Movement::UpdateSliding(float deltaTime, Transform3D& transform, bool isCrouching, bool isCrouchingStarted, const Vector3& moveDir, bool hasMoveInput) {
		if (isCrouching) {
			currentMotion_ = Player::Motion::Crouching;
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

			// 操作入力ではスライド速度を変えず進行方向だけを補間
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

		const float friction = [&]() {
			if (isCrouching) {
				return (movementParams_.slideFriction_);
			} else {
				return (slideReleaseFriction_);
			}
		}();

		// 斜面加速を摩擦で相殺しないよう斜面上では減速を弱める
		ApplySlideFriction(deltaTime, [&]() {
			if (isCrouchingOnSlope) {
				return (movementParams_.slideFriction_ * 0.35f);
			} else {
				return (friction);
			}
		}());
	}

	bool Movement::TryGetSlopeDownDirection(Vector3& outDownDirection) const {
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

		// 斜面角度によって加速量が変わらないよう水平方向を正規化
		const float downLength = std::sqrt(downLengthSq);
		outDownDirection.x /= downLength;
		outDownDirection.z /= downLength;
		return true;
	}

	void Movement::ApplySlideFriction(float deltaTime, float friction) {
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

		// XZ成分へ個別に摩擦を掛けて方向が歪まないよう速度長の比率で減衰
		const float speedScale = nextSpeed / slideSpeed;
		slideVelocity_.x *= speedScale;
		slideVelocity_.z *= speedScale;
	}

	bool Movement::IsHorizontalMoveBlocked(const Vector3& currentPosition) const {
		const float attemptedLengthSq =
			lastAttemptedHorizontalMove_.x * lastAttemptedHorizontalMove_.x +
			lastAttemptedHorizontalMove_.z * lastAttemptedHorizontalMove_.z;
		if (attemptedLengthSq <= kRotationEpsilon) {
			return false;
		}

		const float attemptedLength = std::sqrt(attemptedLengthSq);
		const Vector3 actualHorizontalMove = {
			currentPosition.x - lastMoveStartPosition_.x,
			0.0f,
			currentPosition.z - lastMoveStartPosition_.z
		};

		// 横滑りを前進量に含めないよう希望移動方向への実移動成分だけを比較
		const float actualProgress =
			(actualHorizontalMove.x * lastAttemptedHorizontalMove_.x +
			 actualHorizontalMove.z * lastAttemptedHorizontalMove_.z) / attemptedLength;
		return actualProgress < attemptedLength * kBlockedProgressRate;
	}

	void Movement::StartWallJump() {

		// 壁へ進む入力を押し続けても離脱できるよう歩行速度を上回る背面初速を設定
		const Vector3 backward = CreateHorizontalForward(faceYaw_) * -1.0f;
		const float wallJumpSpeed = std::max(0.0f, movementParams_.moveSpeed_ * moveSpeedMultiplier_) +
			std::max(0.0f, movementParams_.jumpMoveBoostSpeed_);
		jumpMoveVelocity_ = backward * wallJumpSpeed;
		isBunnyHopMomentumActive_ = false;
		slideVelocity_ = {};
		lastMoveDirection_ = {};
		hasMoveInput_ = false;

		// 壁登り時間と残りジャンプ回数を維持し、通常Jumpと同じ入力の二重処理を防止
		velocityY_ = movementParams_.jumpPower_;
		isGrounded_ = false;
		isWallClimbing_ = false;
		jumpStartedThisFrame_ = true;
		currentMotion_ = Player::Motion::Jump;
	}

	void Movement::StartWallClimbTimeoutJump() {

		// 通常Jumpと分離して残り回数を参照または消費せず上向き初速だけを付与
		velocityY_ = movementParams_.jumpPower_;
		isGrounded_ = false;
		jumpStartedThisFrame_ = true;
		currentMotion_ = Player::Motion::Jump;
	}

	void Movement::ApplyJumpMoveBoost(float deltaTime, Transform3D& transform) {
		const float boostSpeedSq = jumpMoveVelocity_.x * jumpMoveVelocity_.x + jumpMoveVelocity_.z * jumpMoveVelocity_.z;
		if (boostSpeedSq < 1e-6f) {
			jumpMoveVelocity_.x = 0.0f;
			jumpMoveVelocity_.z = 0.0f;
			return;
		}

		transform.translate.x += jumpMoveVelocity_.x * deltaTime;
		transform.translate.z += jumpMoveVelocity_.z * deltaTime;

		// 地上の減速は受付時間を考慮した更新側へ集約し、空中では従来の摩擦だけを適用
		if (!isGrounded_) {
			ApplyJumpMoveFriction(deltaTime, jumpMoveBoostFriction_);
		}
	}

	void Movement::UpdateBunnyHopState(float deltaTime) {
		if (!isGrounded_ || isWallClimbing_) {
			hasBunnyHopLanding_ = false;
			return;
		}

		float frictionTime = std::max(0.0f, deltaTime);
		if (hasBunnyHopLanding_) {
			const float previousElapsedTime = timeSinceLanding_;
			timeSinceLanding_ += frictionTime;
			const float window = std::max(0.0f, movementParams_.bunnyHopWindow_);

			// 受付終了をまたぐフレームでも、受付時間を超えた区間だけを減速へ反映
			frictionTime = std::max(0.0f, timeSinceLanding_ - window) -
				std::max(0.0f, previousElapsedTime - window);
		}

		ApplyJumpMoveFriction(frictionTime, movementParams_.bunnyHopGroundFriction_);
	}

	bool Movement::TryAddBunnyHopBoost(const MoveInput& input) {

		// 時間の加算誤差で受付時間ちょうどのJumpを取りこぼさないよう微小誤差を許容
		const float window = std::max(0.0f, movementParams_.bunnyHopWindow_);
		if (!isGrounded_ || !hasBunnyHopLanding_ || input.isCrouching ||
			window <= 0.0f || timeSinceLanding_ > window + kBunnyHopTimeEpsilon) {
			return false;
		}

		// 保持速度が小さい場合も通常Jumpの初速を確保し、成功時だけ遅くなる逆転を防止
		const float normalBoostSpeed = std::max(0.0f, movementParams_.jumpMoveBoostSpeed_);
		if (jumpMoveVelocity_.LengthSq() < normalBoostSpeed * normalBoostSpeed) {
			AddJumpMoveBoost(input);
		}

		// 歩行入力と保持中の初速を合成し、実際に進んでいる水平方向を加速基準に採用
		Vector3 horizontalVelocity = jumpMoveVelocity_;
		if (hasMoveInput_) {
			const float inputLength = std::sqrt(input.move.x * input.move.x + input.move.y * input.move.y);
			horizontalVelocity += lastMoveDirection_ * (movementParams_.moveSpeed_ * moveSpeedMultiplier_ * inputLength);
		}
		const float speedSq = horizontalVelocity.LengthSq();
		if (speedSq < kRotationEpsilon) {
			return false;
		}

		// 歩行速度を含む上限までだけ加速し、既に上限を超える勢いは急減速させず維持
		const float speed = std::sqrt(speedSq);
		const float speedBoost = std::min(
			std::max(0.0f, movementParams_.bunnyHopSpeedBoost_),
			std::max(0.0f, movementParams_.bunnyHopMaxSpeed_ - speed)
		);
		jumpMoveVelocity_ += horizontalVelocity * (speedBoost / speed);
		if (speedBoost > 0.0f) {
			isBunnyHopMomentumActive_ = true;
		}
		return true;
	}

	void Movement::UpdateBunnyHopSteering(float deltaTime, const Vector3& moveDir) {

		// バニーホップで得た勢いだけを旋回対象とし、入力のない空中では慣性を維持
		if (!isBunnyHopMomentumActive_ || !hasMoveInput_ || isWallClimbing_ || deltaTime <= 0.0f) {
			return;
		}

		const float speedSq = jumpMoveVelocity_.LengthSq();
		const float steerRate = std::max(0.0f, movementParams_.bunnyHopSteerRate_);
		if (speedSq < kRotationEpsilon || steerRate <= 0.0f) {
			return;
		}

		// 正反対の入力でも方向補間が相殺されないよう、最短の水平角度差を指数補間
		const float currentYaw = std::atan2(jumpMoveVelocity_.x, jumpMoveVelocity_.z);
		const float targetYaw = std::atan2(moveDir.x, moveDir.z);
		const float yawDelta = std::remainder(targetYaw - currentYaw, 2.0f * std::numbers::pi_v<float>);
		const float inputStrength = std::min(1.0f, std::sqrt(moveDir.LengthSq()));
		const float steerT = 1.0f - std::exp(-steerRate * inputStrength * deltaTime);
		const float steeredYaw = currentYaw + yawDelta * steerT;
		const float speed = std::sqrt(speedSq);
		jumpMoveVelocity_ = { std::sin(steeredYaw) * speed, 0.0f, std::cos(steeredYaw) * speed };
	}

	void Movement::ClampBunnyHopSpeed(const Vector3& moveVelocity) {
		if (!isBunnyHopMomentumActive_) {
			return;
		}

		// 外部補正による歩行速度そのものは妨げず、加速分を含む合算速度だけを制限
		const float maxSpeed = std::max(
			std::max(0.0f, movementParams_.bunnyHopMaxSpeed_),
			std::sqrt(moveVelocity.LengthSq())
		);
		const Vector3 horizontalVelocity = moveVelocity + jumpMoveVelocity_;
		const float speedSq = horizontalVelocity.LengthSq();
		if (speedSq <= maxSpeed * maxSpeed) {
			return;
		}

		jumpMoveVelocity_ = horizontalVelocity * (maxSpeed / std::sqrt(speedSq)) - moveVelocity;
	}

	void Movement::ApplyJumpMoveFriction(float deltaTime, float friction) {
		if (deltaTime <= 0.0f || friction <= 0.0f) {
			return;
		}

		// 各軸への個別減算による方向の歪みを防ぐため、速度長の比率で水平成分を減衰
		const float boostSpeed = std::sqrt(jumpMoveVelocity_.LengthSq());
		const float nextSpeed = std::max(0.0f, boostSpeed - friction * deltaTime);
		if (nextSpeed <= 0.001f) {
			jumpMoveVelocity_.x = 0.0f;
			jumpMoveVelocity_.z = 0.0f;
			isBunnyHopMomentumActive_ = false;
			return;
		}

		const float speedScale = nextSpeed / boostSpeed;
		jumpMoveVelocity_.x *= speedScale;
		jumpMoveVelocity_.z *= speedScale;
	}

	void Movement::AddJumpMoveBoost(const MoveInput& input) {
		if (!hasMoveInput_ || input.isCrouching) {
			return;
		}

		const float boostSpeed = std::max(0.0f, movementParams_.jumpMoveBoostSpeed_);
		float maxBoostSpeed = boostSpeed * 2.0f;
		if (isGrounded_) {
			maxBoostSpeed = boostSpeed;
		}

		// 通常Jumpでは加速上限を維持しつつ、バニーホップや壁Jumpの勢いの切り捨てを防止
		maxBoostSpeed = std::max(maxBoostSpeed, std::sqrt(jumpMoveVelocity_.LengthSq()));
		jumpMoveVelocity_ += lastMoveDirection_ * boostSpeed;
		const float boostSpeedSq = jumpMoveVelocity_.x * jumpMoveVelocity_.x + jumpMoveVelocity_.z * jumpMoveVelocity_.z;
		if (boostSpeedSq <= maxBoostSpeed * maxBoostSpeed) {
			return;
		}

		const float speedScale = maxBoostSpeed / std::sqrt(boostSpeedSq);
		jumpMoveVelocity_.x *= speedScale;
		jumpMoveVelocity_.z *= speedScale;
	}

	void Movement::UpdateKnockback(float deltaTime, Transform3D& transform) {
		if (knockbackVelocity_.LengthSq() <= kKnockbackVelocityEpsilonSq || deltaTime <= 0.0f) {
			return;
		}

		// 指数減衰を積分してフレームレートに依存しない移動距離を算出
		const float decay = std::exp(-kKnockbackDamping * deltaTime);
		const Vector3 knockbackMove = knockbackVelocity_ * ((1.0f - decay) / kKnockbackDamping);
		transform.translate.x += knockbackMove.x;
		transform.translate.z += knockbackMove.z;
		knockbackVelocity_ *= decay;
		if (knockbackVelocity_.LengthSq() <= kKnockbackVelocityEpsilonSq) {
			knockbackVelocity_ = {};
		}
	}

	void Movement::ApplySlopeGroundSnap(float deltaTime, Transform3D& transform) {
		if (!isGrounded_ || velocityY_ > 0.0f || isWallClimbing_) {
			return;
		}

		float slopeCenterY = 0.0f;

		// フレーム内の落下距離を許容範囲へ加えて高速移動時の斜面追従漏れを防止
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

	void Movement::UpdateModelTransform(float deltaTime, Transform3D& transform, Model* model, bool isSlopeGroundContact, float modelYawOffset) {
		if (!model) {
			return;
		}

		Vector3 targetGroundNormal = { 0.0f, 1.0f, 0.0f };
		Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
		if (isSlopeGroundContact && MyCollider::TryGetSlopeGroundNormal(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, slopeNormal)) {
			targetGroundNormal = NormalizeOrFallback(slopeNormal, { 0.0f, 1.0f, 0.0f });
		}

		// フレームレートに依存しない指数補間で斜面法線への追従を平滑化
		const float normalT = std::clamp(1.0f - std::exp(-modelGroundNormalFollowSpeed_ * deltaTime), 0.0f, 1.0f);
		currentGroundNormal_ = NormalizeOrFallback(Math::Lerp(currentGroundNormal_, targetGroundNormal, normalT), targetGroundNormal);
		transform.rotate = CreateSlopeAlignedRotation(faceYaw_, currentGroundNormal_);
		const Vector3 modelRotation = CreateSlopeAlignedRotation(faceYaw_ + modelYawOffset, currentGroundNormal_);

		model->SetPosition(transform.translate);
		model->SetRotation(modelRotation);
	}

	void Movement::Jump(float deltaTime, Transform3D& transform, const MoveInput& input) {

		// 壁登りを接地回復に含めず実際の地面へ接地している場合だけ使用回数を回復
		if (isGrounded_ && !isWallClimbing_) {
			remainingJumpCount_ = GetMaxJumpCount();
		}

		// 壁Jumpが成立したフレームは通常Jumpによる初速加算と回数消費を抑止
		if (!jumpStartedThisFrame_ && !isWallClimbing_ && remainingJumpCount_ > 0 && input.isJumpTriggered) {

			// 着地直後の地上Jumpだけ加速を追加し、空中Jumpでは通常の水平初速処理を使用
			if (!TryAddBunnyHopBoost(input)) {
				AddJumpMoveBoost(input);
			}
			hasBunnyHopLanding_ = false;
			velocityY_ = movementParams_.jumpPower_;
			isGrounded_ = false;
			jumpStartedThisFrame_ = true;
			remainingJumpCount_--;
			Logger::Output("[Engine] ジャンプ開始 残り回数: " + std::to_string(remainingJumpCount_), Logger::Level::Application);
		}

		if (!isGrounded_ && !isWallClimbing_) {
			velocityY_ -= movementParams_.gravity_ * deltaTime;
		}

		transform.translate.y += velocityY_ * deltaTime;

		// Collider接地を取りこぼしても基準地面を貫通しないよう最終補正
		if (transform.translate.y <= groundY_) {
			isBaseGroundContact_ = true;
			transform.translate.y = groundY_;
			velocityY_ = 0.0f;
			if (!isGrounded_) {
				isGrounded_ = true;
				remainingJumpCount_ = GetMaxJumpCount();
			}
		}
	}

	float Movement::GetMaxJumpCount() const {
		return std::max(0.0f, movementParams_.jumpCount_ + static_cast<float>(additionalJumpCount_));
	}
}
