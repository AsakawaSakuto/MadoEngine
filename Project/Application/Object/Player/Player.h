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

	/// @brief Slope上を移動しているときに足元を斜面へ追従させる
	/// @param deltaTime 1フレームの経過時間
	void ApplySlopeGroundSnap(float deltaTime);

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
	float slopeSnapDistance_ = 1.0f;
	int   jumpCount_  = 10;      // ジャンプ可能回数
	int remainingJumpCount_ = 0; // 残りジャンプ回数
};
