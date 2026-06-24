#pragma once
#include "../IGameObject.h"
#include "PlayerController.h"
#include "PlayerStatus.h"

class PlayerMovement {
public:

	/// @brief 移動状態を初期化します。
	void Initialize();

	/// @brief 入力と重力をもとに移動を更新します。
	/// @param deltaTime 1フレームの経過時間です。
	/// @param transform 更新対象のTransformです。
	/// @param camera 移動方向の基準にするCameraです。
	/// @param input Playerの移動操作入力です。
	void Update(float deltaTime, Transform3D& transform, const Camera* camera, const PlayerMoveInput& input);

	/// @brief 接地状態を移動処理へ反映します。
	/// @param isGroundContact 通常地面に接地していればtrueです。
	/// @param isSlopeGroundContact 坂に接地していればtrueです。
	/// @param input Playerの移動操作入力です。
	void SetGroundContact(bool isGroundContact, bool isSlopeGroundContact, const PlayerMoveInput& input);

	/// @brief PlayerのModel座標と回転を接地状態に合わせて更新します。
	/// @param deltaTime 1フレームの経過時間です。
	/// @param transform 更新対象のTransformです。
	/// @param model 更新対象のModelです。
	/// @param isSlopeGroundContact 坂に接地していればtrueです。
	void UpdateModelTransform(float deltaTime, Transform3D& transform, Model* model, bool isSlopeGroundContact);

	/// @brief 現在の動作状態を取得します。
	/// @return 現在の動作状態です。
	PlayerMotion GetCurrentMotion() const { return currentMotion_; }

	/// @brief Y方向速度を取得します。
	/// @return Y方向速度です。
	float GetVelocityY() const { return velocityY_; }

	/// @brief スライディング速度を取得します。
	/// @return スライディング速度です。
	Vector3 GetSlideVelocity() const { return slideVelocity_; }

	/// @brief ジャンプ時の水平初速を取得します。
	/// @return ジャンプ時の水平初速です。
	Vector3 GetJumpMoveVelocity() const { return jumpMoveVelocity_; }

	/// @brief 移動パラメータを取得します。
	/// @return 移動パラメータです。
	PlayerMovementParams& GetParams() { return movementParams_; }

private:

	void Move(float deltaTime, Transform3D& transform, const Camera* camera, const PlayerMoveInput& input);

	void Jump(float deltaTime, Transform3D& transform, const PlayerMoveInput& input);

	/// @brief Slope上を移動しているときに足元を斜面へ追従させます。
	/// @param deltaTime 1フレームの経過時間です。
	/// @param transform 更新対象のTransformです。
	void ApplySlopeGroundSnap(float deltaTime, Transform3D& transform);

	/// @brief ジャンプ時に加えた水平初速を反映します。
	/// @param deltaTime 1フレームの経過時間です。
	/// @param transform 更新対象のTransformです。
	void ApplyJumpMoveBoost(float deltaTime, Transform3D& transform);

	/// @brief 移動入力中のジャンプに水平初速を加えます。
	/// @param input Playerの移動操作入力です。
	void AddJumpMoveBoost(const PlayerMoveInput& input);

	/// @brief Crouching中のスライディング速度を更新します。
	/// @param deltaTime 1フレームの経過時間です。
	/// @param transform 更新対象のTransformです。
	/// @param isCrouching Crouching入力が押されていればtrueです。
	/// @param isCrouchingStarted このフレームでCrouching入力が押され始めたらtrueです。
	/// @param moveDir 入力から計算した水平移動方向です。
	/// @param hasMoveInput 移動入力があればtrueです。
	void UpdateSliding(float deltaTime, Transform3D& transform, bool isCrouching, bool isCrouchingStarted, const Vector3& moveDir, bool hasMoveInput);

	/// @brief 現在接地しているSlopeの下り方向を取得します。
	/// @param outDownDirection 下り方向の出力先です。
	/// @return Slopeの下り方向を取得できればtrueです。
	bool TryGetSlopeDownDirection(Vector3& outDownDirection) const;

	/// @brief 水平スライディング速度に摩擦を適用します。
	/// @param deltaTime 1フレームの経過時間です。
	/// @param friction 減速量です。
	void ApplySlideFriction(float deltaTime, float friction);

private:

	Vector3 slideVelocity_ = { 0.0f, 0.0f, 0.0f };
	Vector3 jumpMoveVelocity_ = { 0.0f, 0.0f, 0.0f };
	Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 0.0f };

	float velocityY_ = 0.0f;
	bool isGrounded_ = false;

	float groundY_ = 0.0f;               // 接地している地面のY座標
	float slopeSnapDistance_ = 1.0f;     // Slopeに足が届いているとみなす距離
	float jumpMoveBoostFriction_ = 0.0f; // ジャンプ時の水平初速に対する摩擦
	float slideReleaseFriction_ = 10.0f; // スライディング解除時の摩擦
	int remainingJumpCount_ = 0;         // 残りジャンプ回数

	PlayerMovementParams movementParams_; // 移動に関するパラメータ

	bool hasMoveInput_ = false; // 移動入力があるか

	float faceYaw_ = 0.0f; // Playerの水平な向き

	float modelGroundNormalFollowSpeed_ = 16.0f; // Model接地法線の補間速度
	Vector3 currentGroundNormal_ = { 0.0f, 1.0f, 0.0f }; // 補間中の接地法線

	PlayerMotion currentMotion_ = PlayerMotion::Idle;
};
