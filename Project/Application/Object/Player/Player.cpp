#include "Player.h"
#include <algorithm>
#include <cmath>

void Player::Initialize() {
	transform_.translate = { 0.0f, 100.0f, 0.0f };
	transform_.SetAllScale(0.5f);

	AABB aabb;
	aabb.min = { -0.5f, 0.0f, -0.5f };
	aabb.max = { 0.5f, 2.0f, 0.5f };
	hitAABB_ = aabb;

	Sphere s;
	s.radius = 0.5f;
	colliderShape_ = s;

	MyCollider::RegisterCollider("PlayerMovementSphere", CollisionTag::PlayerMovementSphere, &colliderShape_, &transform_.translate, 0.0f);
	MyCollider::RegisterCollider("PlayerHitBox", CollisionTag::PlayerHitBox, &hitAABB_, &transform_.translate, 0.0f);

	model_ = MyModel::Create("Player", "walk",SceneType::Test);
	model_->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
	model_->SetTexture("white16x16");

	currentMotion_ = PlayerMotion::Idle;
}

void Player::Update(float deltaTime) {
	// 先に入力移動と、重力による落下処理を行う
	Move(deltaTime);
	Jump(deltaTime);
	ApplySlopeGroundSnap(deltaTime);

	transform_.translate.x = std::clamp(transform_.translate.x, -7.5f, 292.5f);
	transform_.translate.y = std::clamp(transform_.translate.y, 0.0f, 100.0f);
	transform_.translate.z = std::clamp(transform_.translate.z, -7.5f, 292.5f);

	// 移動後の位置で押し戻しを行い、描画位置にも解決後の座標を反映する
	MyCollider::Update();

	// 床面接触（Y軸が最小解決軸）かどうかを判定する
	bool isGroundContact = MyCollider::IsGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapBlock);
	bool isSlopeGroundContact = MyCollider::IsSlopeGroundContact(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope);

	if (isGroundContact || isSlopeGroundContact) {
		// AABBの上面に乗っている → 接地
		if (velocityY_ < 0.0f) {
			velocityY_ = 0.0f;
		}
		isGrounded_ = true;

		if (!MyInput::Press("Crouching")) {
			slideVelocity_ = { 0.0f, 0.0f, 0.0f };
		}
		if (velocityY_ <= 0.0f) {
			jumpMoveVelocity_ = { 0.0f, 0.0f, 0.0f };
		}
	} else {
		// 床面接触なし（側面のみ接触 or 空中）→ 空中扱いにして重力を継続させる
		isGrounded_ = false;
	}

	UpdateModelTransform(isSlopeGroundContact);

	model_->SetTransform(transform_);

	if (currentMotion_ == PlayerMotion::Crouching) {
		model_->SetColor({ 1.0f,0.0f,0.0f,1.0f });
	} else {
		model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
	}

	if (MyInput::GetKeybord()->IsTrigger(DIK_F3)) {
		transform_.translate = { 0.0f,100.0f,0.0f };
	}

	// デバッグ表示
	Vector4 color = { 0.0f,0.0f,0.0f,1.0f };
	MyDebugLine::AddShape(std::get<AABB>(hitAABB_), color);
	MyDebugLine::AddShape(std::get<Sphere>(colliderShape_), color);
}

void Player::Move(float deltaTime) {
	hasMoveInput_ = false;
	lastMoveDirection_ = { 0.0f, 0.0f, 0.0f };

	if (!camera_) {
		return;
	}

	const bool isCrouching = MyInput::Press("Crouching");
	const bool isCrouchingStarted = isCrouching && !wasCrouching_;
	Vector2 input = MyInput::GetGamePad()->GetLeftStick();
	input.x += MyInput::Press("Right") ? 1.0f : 0.0f;
	input.x -= MyInput::Press("Left") ? 1.0f : 0.0f;
	input.y += MyInput::Press("Up") ? 1.0f : 0.0f;
	input.y -= MyInput::Press("Down") ? 1.0f : 0.0f;

	const float inputLengthSq = input.x * input.x + input.y * input.y;
	bool hasMoveInput = inputLengthSq >= 1e-5f;
	if (hasMoveInput && inputLengthSq > 1.0f) {
		const float inputLength = std::sqrt(inputLengthSq);
		input.x /= inputLength;
		input.y /= inputLength;
	}

	// カメラのY軸回転（ヨー）からXZ平面上の前方・右方ベクトルを算出
	const float yaw = camera_->GetRotation().y;
	const Vector3 forward = { std::sin(yaw),  0.0f, std::cos(yaw) };
	const Vector3 right   = { std::cos(yaw),  0.0f, -std::sin(yaw) };

	// 左スティックの入力をカメラ基準のXZ方向に変換
	Vector3 moveDir = {
		forward.x * input.y + right.x * input.x,
		0.0f,
		forward.z * input.y + right.z * input.x
	};

	const float moveLengthSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
	if (moveLengthSq > 1e-5f) {
		const float moveLength = std::sqrt(moveLengthSq);
		lastMoveDirection_ = { moveDir.x / moveLength, 0.0f, moveDir.z / moveLength };
		hasMoveInput_ = true;
		transform_.rotate.y = std::atan2(moveDir.x, moveDir.z);
	} else {
		hasMoveInput = false;
	}

	if (!isCrouching && hasMoveInput) {
		transform_.translate.x += moveDir.x * movementParams_.moveSpeed_ * deltaTime;
		transform_.translate.z += moveDir.z * movementParams_.moveSpeed_ * deltaTime;
		currentMotion_ = PlayerMotion::Walk;
	} else if (!isCrouching && !isGrounded_) {
		currentMotion_ = PlayerMotion::Jump;
	} else if (!isCrouching) {
		currentMotion_ = PlayerMotion::Idle;
	}

	if (!isCrouching) {
		ApplyJumpMoveBoost(deltaTime);
	}

	UpdateSliding(deltaTime, isCrouching, isCrouchingStarted, moveDir, hasMoveInput);
	wasCrouching_ = isCrouching;
}

/// @brief Crouching中のスライディング速度を更新する
/// @param deltaTime 1フレームの経過時間
/// @param isCrouching Crouching入力が押されていればtrue
/// @param isCrouchingStarted このフレームでCrouching入力が押され始めたらtrue
/// @param moveDir 入力から計算した水平移動方向
/// @param hasMoveInput 移動入力があればtrue
void Player::UpdateSliding(float deltaTime, bool isCrouching, bool isCrouchingStarted, const Vector3& moveDir, bool hasMoveInput) {
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

	transform_.translate.x += slideVelocity_.x * deltaTime;
	transform_.translate.z += slideVelocity_.z * deltaTime;

	if (std::abs(slideVelocity_.x) > 0.001f || std::abs(slideVelocity_.z) > 0.001f) {
		transform_.rotate.y = std::atan2(slideVelocity_.x, slideVelocity_.z);
	}

	const float friction = isCrouching ? movementParams_.slideFriction_ : slideReleaseFriction_;
	ApplySlideFriction(deltaTime, isCrouchingOnSlope ? movementParams_.slideFriction_ * 0.35f : friction);
}

/// @brief 現在接地しているSlopeの下り方向を取得する
/// @param outDownDirection 下り方向の出力先
/// @return Slopeの下り方向を取得できればtrue
bool Player::TryGetSlopeDownDirection(Vector3& outDownDirection) const {
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

/// @brief 水平スライディング速度に摩擦を適用する
/// @param deltaTime 1フレームの経過時間
/// @param friction 減速量
void Player::ApplySlideFriction(float deltaTime, float friction) {
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

/// @brief ジャンプ時に加えた水平初速を反映する
/// @param deltaTime 1フレームの経過時間
void Player::ApplyJumpMoveBoost(float deltaTime) {
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

	transform_.translate.x += jumpMoveVelocity_.x * deltaTime;
	transform_.translate.z += jumpMoveVelocity_.z * deltaTime;

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

/// @brief 移動入力中のジャンプに水平初速を加える
void Player::AddJumpMoveBoost() {
	if (!hasMoveInput_ || MyInput::Press("Crouching")) {
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

/// @brief Slope上を移動しているときに足元を斜面へ追従させる
/// @param deltaTime 1フレームの経過時間
void Player::ApplySlopeGroundSnap(float deltaTime) {
	if (!isGrounded_ || velocityY_ > 0.0f) {
		return;
	}

	float slopeCenterY = 0.0f;
	float snapDistance = slopeSnapDistance_ + movementParams_.gravity_ * deltaTime * deltaTime;
	if (!MyCollider::TryGetSlopeGroundCenterY(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, slopeCenterY, snapDistance)) {
		return;
	}

	if (transform_.translate.y > slopeCenterY) {
		transform_.translate.y = slopeCenterY;
		if (velocityY_ < 0.0f) {
			velocityY_ = 0.0f;
		}
		isGrounded_ = true;
	}
}

/// @brief PlayerのModel座標と回転を現在の接地状態に合わせて更新する
/// @param isSlopeGroundContact Slope上面に接地していればtrue
void Player::UpdateModelTransform(bool isSlopeGroundContact) {
	if (!model_) {
		return;
	}

	transform_.rotate.x = 0.0f;
	transform_.rotate.z = 0.0f;

	Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
	if (isSlopeGroundContact && MyCollider::TryGetSlopeGroundNormal(CollisionTag::PlayerMovementSphere, CollisionTag::MapSlope, slopeNormal)) {
		const float cosYaw = std::cos(transform_.rotate.y);
		const float sinYaw = std::sin(transform_.rotate.y);
		Vector3 localNormal = {
			slopeNormal.x * cosYaw - slopeNormal.z * sinYaw,
			slopeNormal.y,
			slopeNormal.x * sinYaw + slopeNormal.z * cosYaw
		};

		transform_.rotate.x = std::atan2(localNormal.z, localNormal.y);
		transform_.rotate.z = std::atan2(-localNormal.x, localNormal.y);
	}

	model_->SetPosition(transform_.translate);
	model_->SetRotation(transform_.rotate);
}

void Player::Jump(float deltaTime) {
	// 着地時にジャンプ回数をリセット
	if (isGrounded_) {
		remainingJumpCount_ = movementParams_.jumpCount_;
	}

	// ジャンプ入力（Aボタン）：残りジャンプ回数があれば受け付ける
	if (remainingJumpCount_ > 0 && MyInput::Trigger("Jump")) {
		velocityY_  = movementParams_.jumpPower_;
		isGrounded_ = false;
		AddJumpMoveBoost();
		remainingJumpCount_--;
		Logger::Output("ジャンプ開始 残り回数: " + std::to_string(remainingJumpCount_), Logger::Level::Application);
	}

	// 重力を加算
	if (!isGrounded_) {
		velocityY_ -= movementParams_.gravity_ * deltaTime;
	}

	// Y座標に速度を反映
	transform_.translate.y += velocityY_ * deltaTime;

	// 地面着地判定
	if (transform_.translate.y <= groundY_) {
		transform_.translate.y = groundY_;
		velocityY_  = 0.0f;
		if (!isGrounded_) {
			isGrounded_ = true;
			remainingJumpCount_  = movementParams_.jumpCount_;
			//Logger::Output("着地", Logger::Level::Application);
		}
	}
}

void Player::DrawImGui() {

#ifdef USE_IMGUI

	ImGui::Begin("プレイヤー");
	const float horizontalVelocityX = slideVelocity_.x + jumpMoveVelocity_.x;
	const float horizontalVelocityZ = slideVelocity_.z + jumpMoveVelocity_.z;
	const float horizontalSpeed = std::sqrt(horizontalVelocityX * horizontalVelocityX + horizontalVelocityZ * horizontalVelocityZ);
	const float jumpMoveBoostSpeed = std::sqrt(jumpMoveVelocity_.x * jumpMoveVelocity_.x + jumpMoveVelocity_.z * jumpMoveVelocity_.z);
	const float currentSpeed = std::sqrt(horizontalSpeed * horizontalSpeed + velocityY_ * velocityY_);
	ImGui::Text("現在の動作: %s", ToMotionText(currentMotion_));
	ImGui::Text("速度: %.2f", currentSpeed);
	ImGui::Text("水平速度: %.2f", horizontalSpeed);
	ImGui::Text("Y速度: %.2f", velocityY_);
	ImGui::Text("ジャンプ横初速: %.2f", jumpMoveBoostSpeed);
	ImGui::Separator();
	ImGui::DragFloat("移動速度", &movementParams_.moveSpeed_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("ジャンプ力", &movementParams_.jumpPower_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("重力", &movementParams_.gravity_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("スライド開始速度", &movementParams_.slideStartSpeed_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("スライド方向補正率", &movementParams_.slideSteerRate_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("斜面スライド加速度", &movementParams_.slopeSlideAcceleration_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("最大スライド速度", &movementParams_.maxSlideSpeed_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("スライド摩擦", &movementParams_.slideFriction_, 0.1f, 0.0f, 100.0f);
	ImGui::DragInt("ジャンプ回数", &movementParams_.jumpCount_, 1, 0, 10);
	ImGui::DragFloat("ジャンプ横初速", &movementParams_.jumpMoveBoostSpeed_, 0.1f, 0.0f, 100.0f);
	ImGui::End();

	ImGui::Begin("プレイヤーステータス");

	ImGui::Text("Health : %d / %d", status_.currentHealth, status_.maxHealth);
	ImGui::Text("Shield : %d / %d", status_.currentShield, status_.maxShield);
	ImGui::Separator();
	ImGui::Text("Lv : %d", status_.level);
	ImGui::Text("CurrentExp : %d", status_.currentExp);
	ImGui::Text("NextLvExp  : %d", status_.expToNextLevel);
	ImGui::Separator();
	ImGui::Text("Money : %d", status_.currentMoney);

	ImGui::End();

#endif // USE_IMGUI
}
