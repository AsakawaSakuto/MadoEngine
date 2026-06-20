#pragma once
#include "../IGameObject.h"
#include "PlayerStatus.h"

class Player : public IGameObject {
public:

	void Initialize() override;

	void Update(float deltaTime) override;

	Vector3 GetPosition() const { return transform_.translate; }

	void SetCamera(Camera* camera) { camera_ = camera; }

	/// @brief 所持金を加算します。
	/// @param amount 加算する所持金です。
	void AddMoney(int amount);
	
	void DrawImGui();

private:

	/// @brief Slope上を移動しているときに足元を斜面へ追従させる
	/// @param deltaTime 1フレームの経過時間
	void ApplySlopeGroundSnap(float deltaTime);

	/// @brief PlayerのModel座標と回転を現在の接地状態に合わせて更新する
	/// @param isSlopeGroundContact Slope上面に接地していればtrue
	/// @param deltaTime 1フレームの経過時間
	void UpdateModelTransform(float deltaTime, bool isSlopeGroundContact);

	void Move(float deltaTime);

	void Jump(float deltaTime);

	/// @brief ジャンプ時に加えた水平初速を反映する
	/// @param deltaTime 1フレームの経過時間
	void ApplyJumpMoveBoost(float deltaTime);

	/// @brief 移動入力中のジャンプに水平初速を加える
	void AddJumpMoveBoost();

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

	Vector3 slideVelocity_ = { 0.0f, 0.0f, 0.0f };
	Vector3 jumpMoveVelocity_ = { 0.0f, 0.0f, 0.0f };
	Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 0.0f };

	ColliderShape hitAABB_;

	Camera* camera_ = nullptr;

	float velocityY_  = 0.0f;
	bool  isGrounded_ = false;

	// --- 調整用パラメータ ---
	float groundY_ = 0.0f;               // 接地している地面のY座標
	float slopeSnapDistance_ = 1.0f;     // Slopeに足が届いているとみなす距離
	float jumpMoveBoostFriction_ = 0.0f; // ジャンプ時の水平初速に対する摩擦（0なら減速なし）
	float slideReleaseFriction_ = 10.0f; // スライディング解除時の摩擦
	int remainingJumpCount_ = 0;         // 残りジャンプ回数

	PlayerStatus status_;                     // ステータス
	PlayerStatusMultiplier statusMultiplier_; // ステータスの倍率
	PlayerMovementParams movementParams_;     // 移動に関するパラメータ

	bool wasCrouching_ = false; // 前フレームのCrouching入力状態
	bool hasMoveInput_ = false; // 移動入力があるか

	float faceYaw_ = 0.0f;      // Playerの水平な向き

	float modelGroundNormalFollowSpeed_ = 16.0f; // Model接地法線の補間速度
	Vector3 currentGroundNormal_ = { 0.0f, 1.0f, 0.0f }; // 補間中の接地法線

	PlayerMotion currentMotion_ = PlayerMotion::Idle;
};
