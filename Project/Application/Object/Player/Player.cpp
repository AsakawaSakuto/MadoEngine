#include "Player.h"

void Player::Initialize() {
	position_   = { 0.0f, 0.0f, 0.0f };
	velocityY_  = 0.0f;
	isGrounded_ = true;

	Sphere s;
	s.radius = 1.0f;
	hitbox_ = s;

	ColliderManager::GetInstance()->RegisterCollider("PlayerSphere", "Sphere", &hitbox_, &position_, nullptr);
}

void Player::Update() {
	
	if (MyInput::Press("UP")) {
		position_.z += 1.0f * 1.0f / 60.0f;
	}
	if (MyInput::Press("DOWN")) {
		position_.z -= 1.0f * 1.0f / 60.0f;
	}
	if (MyInput::Press("LEFT")) {
		position_.x -= 1.0f * 1.0f / 60.0f;
	}
	if (MyInput::Press("RIGHT")) {
		position_.x += 1.0f * 1.0f / 60.0f;
	}
	if (MyInput::Press("E")) {
		position_.y -= 1.0f * 1.0f / 60.0f;
	}
	if (MyInput::Press("Q")) {
		position_.y += 1.0f * 1.0f / 60.0f;
	}
	// ジャンプ
	if (MyInput::Trigger("Jump") && isGrounded_) {
		velocityY_  = kJumpPower_;
		isGrounded_ = false;
	}

	// 重力・垂直移動
	if (!isGrounded_) {
		velocityY_   += kGravity_ * (1.0f / 60.0f);
		position_.y  += velocityY_ * (1.0f / 60.0f);

		// 着地判定
		if (position_.y <= kGroundY_) {
			position_.y = kGroundY_;
			velocityY_  = 0.0f;
			isGrounded_ = true;
		}
	}

	Vector4 color;
	if (ColliderManager::GetInstance()->IsHitTags("Player", "AABB")) {
		color = { 0.0f,1.0f,0.0f,1.0f };
	} else {
		color = { 1.0f,1.0f,0.0f,1.0f };
	}

	std::get<Sphere>(hitbox_).center = position_;
	MyDebugLine::AddShape(std::get<Sphere>(hitbox_), color);
}