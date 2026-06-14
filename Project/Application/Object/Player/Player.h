#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MathHeaders.h"
#include "PlayerStatus.h"

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

	/// @brief PlayerのModel座標と回転を現在の接地状態に合わせて更新する
	/// @param isSlopeGroundContact Slope上面に接地していればtrue
	void UpdateModelTransform(bool isSlopeGroundContact);

	void Move(float deltaTime);

	void Jump(float deltaTime);

	/// @brief Crouching中のスライディング速度を更新する
	/// @param deltaTime 1フレームの経過時間
	/// @param isCrouching Crouching入力が押されていればtrue
	/// @param isCrouchingStarted このフレームでCrouching入力が押され始めたらtrue
	/// @param moveDir 入力から計算した水平移動方向
	/// @param hasMoveInput 移動入力があればtrue
	void UpdateSliding(float deltaTime, bool isCrouching, bool isCrouchingStarted, const Vector3& moveDir, bool hasMoveInput);

	/// @brief 現在接地しているSlopeの下り方向を取得する
	/// @param outDownDirection 下り方向の出力先
	/// @return Slopeの下り方向を取得できればtrue
	bool TryGetSlopeDownDirection(Vector3& outDownDirection) const;

	/// @brief 水平スライディング速度に摩擦を適用する
	/// @param deltaTime 1フレームの経過時間
	/// @param friction 減速量
	void ApplySlideFriction(float deltaTime, float friction);

private:

	Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
	Vector3 scale_ = { 0.5f, 0.5f, 0.5f };
	Vector3 slideVelocity_ = { 0.0f, 0.0f, 0.0f };

	Shape hitAABB_;
	Shape hitSphere_;

	Camera* camera_ = nullptr;

	Model* model_ = nullptr;

	float velocityY_  = 0.0f;
	bool  isGrounded_ = false;

	float moveSpeed_  = 5.0f;
	float dashSpeed_  = 30.0f;
	float jumpPower_  = 15.0f;
	float gravity_    = 20.0f;
	float groundY_    = 0.0f;
	float slopeSnapDistance_ = 1.0f;
	float slideStartSpeed_ = 7.5f;
	float slideSteerRate_ = 5.0f;
	float slopeSlideAcceleration_ = 30.0f;
	float maxSlideSpeed_ = 25.0f;
	float slideFriction_ = 5.0f;
	float slideReleaseFriction_ = 10.0f;
	int   jumpCount_  = 10;      // ジャンプ可能回数
	int remainingJumpCount_ = 0; // 残りジャンプ回数
	bool wasCrouching_ = false;

	PlayerMotion currentMotion_ = PlayerMotion::Idle;
};
