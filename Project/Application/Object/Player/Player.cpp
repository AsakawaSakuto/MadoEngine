#include "Player.h"

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
	//MyCollider::RegisterCollider("PlayerAABB", CollisionTag::PlayerAABB, &hitAABB_, &position_, 0.0f);
}

void Player::Update(float deltaTime) {
	// 先に入力移動と、重力による落下処理を行う
	Move(deltaTime);
	Jump(deltaTime);

	// 床面接触（Y軸が最小解決軸）かどうかを判定する
	bool isGroundContact = MyCollider::IsGroundContact("PlayerSphere", CollisionTag::MapBlock);

	if (isGroundContact) {
		// AABBの上面に乗っている → 接地
		if (velocityY_ < 0.0f) {
			velocityY_ = 0.0f;
		}
		isGrounded_ = true;
	} else {
		// 床面接触なし（側面のみ接触 or 空中）→ 空中扱いにして重力を継続させる
		isGrounded_ = false;
	}

	// デバッグ表示
	Vector4 color = { 1.0f,1.0f,0.0f,1.0f };
	MyDebugLine::AddShape(std::get<AABB>(hitAABB_), color);
	MyDebugLine::AddShape(std::get<Sphere>(hitSphere_), color);
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

	const float speed = MyInput::Press("Dash") ? kDashSpeed : kMoveSpeed;
	position_.x += moveDir.x * speed * deltaTime;
	position_.y += moveDir.y * speed * deltaTime;
	position_.z += moveDir.z * speed * deltaTime;
}

void Player::Jump(float deltaTime) {
	// 着地時にジャンプ回数をリセット
	if (isGrounded_) {
		jumpCount_ = kJumpCount;
	}

	// ジャンプ入力（Aボタン）：残りジャンプ回数があれば受け付ける
	if (jumpCount_ > 0 && MyInput::Trigger("Jump")) {
		velocityY_  = kJumpPower;
		isGrounded_ = false;
		jumpCount_--;
		Logger::Output("ジャンプ開始 残り回数: " + std::to_string(jumpCount_), Logger::Level::Application);
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
			jumpCount_  = kJumpCount;
			Logger::Output("着地", Logger::Level::Application);
		}
	}
}