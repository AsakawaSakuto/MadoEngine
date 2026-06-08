#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MathHeaders.h"

class Player {
public:

	void Initialize();

	void Update(float deltaTime);

	Vector3 GetPosition() const { return position_; }

	/// @brief 移動の基準となるカメラをセットする
	/// @param camera カメラへのポインタ
	void SetCamera(Camera* camera) { camera_ = camera; }

private:

	void Move(float deltaTime);

	void Jump(float deltaTime);

private:

	Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	Shape hitAABB_;
	Shape hitSphere_;

	Camera* camera_ = nullptr;

	Model* model_ = nullptr;

	float velocityY_  = 0.0f;
	bool  isGrounded_ = false;

	static constexpr float kMoveSpeed  = 5.0f;
	static constexpr float kDashSpeed  = 10.0f;
	static constexpr float kJumpPower  = 8.0f;
	static constexpr float kGravity    = 20.0f;
	static constexpr float kGroundY    = 0.0f;
	static constexpr int   kJumpCount  = 10;    // ジャンプ可能回数
	int jumpCount_ = 0;                        // 残りジャンプ回数
};