#include "Player.h"

void Player::Initialize() {
	position_ = { 0.0f, 0.0f, 0.0f };

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

	Vector4 color;
	if (ColliderManager::GetInstance()->IsHitTags("Player", "AABB")) {
		color = { 0.0f,1.0f,0.0f,1.0f };
	} else {
		color = { 1.0f,1.0f,0.0f,1.0f };
	}

	std::get<Sphere>(hitbox_).center = position_;
	MyDebugLine::AddShape(std::get<Sphere>(hitbox_), color);
}