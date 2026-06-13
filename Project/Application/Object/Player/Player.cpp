#include "Player.h"
#include <cmath>

void Player::Initialize() {
	position_   = { -10.0f, 0.0f, -10.0f };

	AABB aabb;
	aabb.min = { -0.5f, 0.0f, -0.5f };
	aabb.max = { 0.5f, 2.0f, 0.5f };
	hitAABB_ = aabb;

	Sphere s;
	s.radius = 0.5f;
	hitSphere_ = s;

	MyCollider::RegisterCollider("PlayerSphere", CollisionTag::PlayerSphere, &hitSphere_, &position_, 0.0f);
	MyCollider::RegisterCollider("PlayerAABB", CollisionTag::PlayerAABB, &hitAABB_, &position_, 0.0f);

	model_ = MyModel::Create("Player", "walk",SceneType::Test);
	model_->SetRenderLayer(MadoEngine::Render::RenderLayer::Player);
	//model_->SetTexture("white16x16");

	currentMotion_ = PlayerMotion::Idle;
}

void Player::Finalize() {

	Logger::Output("Player : 登録済みColliderとModelを破棄しました", Logger::Level::Application);
}

void Player::Update(float deltaTime) {
	// 先に入力移動と、重力による落下処理を行う
	Move(deltaTime);
	Jump(deltaTime);
	ApplySlopeGroundSnap(deltaTime);

	// 移動後の位置で押し戻しを行い、描画位置にも解決後の座標を反映する
	MyCollider::Update();

	// 床面接触（Y軸が最小解決軸）かどうかを判定する
	bool isGroundContact = MyCollider::IsGroundContact("PlayerSphere", CollisionTag::MapBlock);
	bool isSlopeGroundContact = MyCollider::IsSlopeGroundContact("PlayerSphere", CollisionTag::MapSlope);

	if (isGroundContact || isSlopeGroundContact) {
		// AABBの上面に乗っている → 接地
		if (velocityY_ < 0.0f) {
			velocityY_ = 0.0f;
		}
		isGrounded_ = true;
	} else {
		// 床面接触なし（側面のみ接触 or 空中）→ 空中扱いにして重力を継続させる
		isGrounded_ = false;
	}

	UpdateModelTransform(isSlopeGroundContact);

	// デバッグ表示
	Vector4 color = { 0.0f,0.0f,0.0f,1.0f };
	MyDebugLine::AddShape(std::get<AABB>(hitAABB_), color);
	MyDebugLine::AddShape(std::get<Sphere>(hitSphere_), color);
}

void Player::Move(float deltaTime) {
	if (!camera_) {
		return;
	}

	Vector2 input = MyInput::GetGamePad()->GetLeftStick();
	input.x += MyInput::Press("Right") ? 1.0f : 0.0f;
	input.x -= MyInput::Press("Left") ? 1.0f : 0.0f;
	input.y += MyInput::Press("Up") ? 1.0f : 0.0f;
	input.y -= MyInput::Press("Down") ? 1.0f : 0.0f;

	const float inputLengthSq = input.x * input.x + input.y * input.y;
	if (inputLengthSq < 1e-5f) {
		return;
	}
	if (inputLengthSq > 1.0f) {
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
		rotate_.y = std::atan2(moveDir.x, moveDir.z);
	}

	const float speed = MyInput::Press("Dash") ? dashSpeed_ : moveSpeed_;
	position_.x += moveDir.x * speed * deltaTime;
	position_.y += moveDir.y * speed * deltaTime;
	position_.z += moveDir.z * speed * deltaTime;
}

/// @brief Slope上を移動しているときに足元を斜面へ追従させる
/// @param deltaTime 1フレームの経過時間
void Player::ApplySlopeGroundSnap(float deltaTime) {
	if (!isGrounded_ || velocityY_ > 0.0f) {
		return;
	}

	float slopeCenterY = 0.0f;
	float snapDistance = slopeSnapDistance_ + gravity_ * deltaTime * deltaTime;
	if (!MyCollider::TryGetSlopeGroundCenterY("PlayerSphere", CollisionTag::MapSlope, slopeCenterY, snapDistance)) {
		return;
	}

	if (position_.y > slopeCenterY) {
		position_.y = slopeCenterY;
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

	rotate_.x = 0.0f;
	rotate_.z = 0.0f;

	Vector3 slopeNormal = { 0.0f, 1.0f, 0.0f };
	if (isSlopeGroundContact && MyCollider::TryGetSlopeGroundNormal("PlayerSphere", CollisionTag::MapSlope, slopeNormal)) {
		const float cosYaw = std::cos(rotate_.y);
		const float sinYaw = std::sin(rotate_.y);
		Vector3 localNormal = {
			slopeNormal.x * cosYaw - slopeNormal.z * sinYaw,
			slopeNormal.y,
			slopeNormal.x * sinYaw + slopeNormal.z * cosYaw
		};

		rotate_.x = std::atan2(localNormal.z, localNormal.y);
		rotate_.z = std::atan2(-localNormal.x, localNormal.y);
	}

	model_->SetPosition(position_);
	model_->SetRotation(rotate_);
}

void Player::Jump(float deltaTime) {
	// 着地時にジャンプ回数をリセット
	if (isGrounded_) {
		remainingJumpCount_ = jumpCount_;
	}

	// ジャンプ入力（Aボタン）：残りジャンプ回数があれば受け付ける
	if (remainingJumpCount_ > 0 && MyInput::Trigger("Jump")) {
		velocityY_  = jumpPower_;
		isGrounded_ = false;
		remainingJumpCount_--;
		Logger::Output("ジャンプ開始 残り回数: " + std::to_string(remainingJumpCount_), Logger::Level::Application);
	}

	// 重力を加算
	if (!isGrounded_) {
		velocityY_ -= gravity_ * deltaTime;
	}

	// Y座標に速度を反映
	position_.y += velocityY_ * deltaTime;

	// 地面着地判定
	if (position_.y <= groundY_) {
		position_.y = groundY_;
		velocityY_  = 0.0f;
		if (!isGrounded_) {
			isGrounded_ = true;
			remainingJumpCount_  = jumpCount_;
			//Logger::Output("着地", Logger::Level::Application);
		}
	}
}

void Player::DrawImGui() {

#ifdef USE_IMGUI

	ImGui::Begin("Player");
	ImGui::DragFloat3("Position", &position_.x, 0.1f);
	ImGui::DragFloat("Move Speed", &moveSpeed_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Dash Speed", &dashSpeed_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Jump Power", &jumpPower_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Gravity", &gravity_, 0.1f, 0.0f, 100.0f);
	ImGui::DragInt("Jump Count", &jumpCount_, 1, 0, 10);
	ImGui::End();

#endif // USE_IMGUI
}
