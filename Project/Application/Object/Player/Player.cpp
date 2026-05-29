#include "Player.h"

void Player::Initialize() {
	position_   = { 0.0f, 0.0f, 0.0f };

	/*AABB s;
	s.min = { -0.5f, 0.0f, -0.5f };
	s.max = { 0.5f, 2.0f, 0.5f };
	hitbox_ = s;*/

	Sphere s;
	s.radius = 0.5f;
	hitbox_ = s;

	MyCollider::RegisterCollider("PlayerSphere", CollisionTag::Player, &hitbox_, &position_, 0.5f);
}

void Player::Update(float deltaTime) {
	
	Move(deltaTime);
	Jump(deltaTime);

	Vector4 color = { 1.0f,1.0f,0.0f,1.0f };
	MyDebugLine::AddShape(std::get<Sphere>(hitbox_), color);
}

void Player::Move(float deltaTime) {
	if (!camera_) {
		return;
	}

	const Vector2 stick = MyInput::GetGamePad()->GetLeftStick();
	if (std::abs(stick.x) < 1e-5f && std::abs(stick.y) < 1e-5f) {
		return;
	}

	// カメラのY軸回転（ヨー）からXZ平面上の前方・右方ベクトルを算出
	const float yaw = camera_->GetRotation().y;
	const Vector3 forward = { std::sin(yaw),  0.0f, std::cos(yaw) };
	const Vector3 right   = { std::cos(yaw),  0.0f, -std::sin(yaw) };

	// 左スティックの入力をカメラ基準のXZ方向に変換
	Vector3 moveDir = {
		forward.x * stick.y + right.x * stick.x,
		0.0f,
		forward.z * stick.y + right.z * stick.x
	};

	position_.x += moveDir.x * kMoveSpeed * deltaTime;
	position_.y += moveDir.y * kMoveSpeed * deltaTime;
	position_.z += moveDir.z * kMoveSpeed * deltaTime;
}

void Player::Jump(float deltaTime) {
	// ジャンプ入力（Aボタン）：接地中のみ受け付ける
	if (isGrounded_ && MyInput::Trigger("Jump")) {
		velocityY_  = kJumpPower;
		isGrounded_ = false;
		Logger::Output("ジャンプ開始", Logger::Level::Application);
	}

	// 重力を加算
	if (!isGrounded_) {
		velocityY_ -= kGravity * deltaTime;
	}

	// Y座標に速度を反映
	position_.y += velocityY_ * deltaTime;

	// 地面着地判定
	if (position_.y <= kGroundY) {
		position_.y = kGroundY;
		velocityY_  = 0.0f;
		if (!isGrounded_) {
			isGrounded_ = true;
			Logger::Output("着地", Logger::Level::Application);
		}
	}
}