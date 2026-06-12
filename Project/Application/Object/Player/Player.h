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

	/// @brief Playerが登録したリソースとColliderを破棄する
	void Finalize();

	void DrawImGui();

private:

	void Move(float deltaTime);

	void Jump(float deltaTime);

private:

	Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };

	Shape hitAABB_;
	Shape hitSphere_;

	Camera* camera_ = nullptr;

	Model* model_ = nullptr;

	float velocityY_  = 0.0f;
	bool  isGrounded_ = false;

	float moveSpeed_  = 30.0f;
	float dashSpeed_  = 10.0f;
	float jumpPower_  = 20.0f;
	float gravity_    = 20.0f;
	float groundY_    = 0.0f;
	int   jumpCount_  = 10;      // ジャンプ可能回数
	int remainingJumpCount_ = 0; // 残りジャンプ回数
};
