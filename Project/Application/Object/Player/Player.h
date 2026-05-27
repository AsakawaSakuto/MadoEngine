#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MathHeaders.h"

class Player {
public:

	void Initialize();

	void Update();

private:
	static constexpr float kGravity_    = -9.8f;  // 重力加速度
	static constexpr float kJumpPower_  = 4.0f;   // ジャンプ初速
	static constexpr float kGroundY_    = 0.0f;   // 地面のY座標

	Vector3 position_  = { 0.0f, 0.0f, 0.0f };
	float   velocityY_ = 0.0f;   // 垂直方向の速度
	bool    isGrounded_ = true;  // 接地フラグ

	Shape hitbox_;
};