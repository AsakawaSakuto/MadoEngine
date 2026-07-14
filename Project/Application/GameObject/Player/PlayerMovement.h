#pragma once
#include "../IGameObject.h"
#include "PlayerController.h"
#include "PlayerStatus.h"

namespace Player {

	/// @brief Playerの移動処理を担当するクラス
	class Movement {
	public:

		/// @brief 移動状態を初期化します。
		void Initialize();

		/// @brief 入力と重力をもとに移動を更新
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		/// @param camera 移動方向の基準にするCamera
		/// @param input Playerの移動操作入力
		void Update(float deltaTime, Transform3D& transform, const Camera* camera, const MoveInput& input);

		/// @brief 接地状態を移動処理へ反映
		/// @param isGroundContact 通常地面に接地していればtrue
		/// @param isSlopeGroundContact 坂に接地していればtrue
		/// @param input Playerの移動操作入力
		void SetGroundContact(bool isGroundContact, bool isSlopeGroundContact, const MoveInput& input);

		/// @brief PlayerのModel座標と回転を接地状態に合わせて更新
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		/// @param model 更新対象のModel
		/// @param isSlopeGroundContact 坂に接地していればtrue
		void UpdateModelTransform(float deltaTime, Transform3D& transform, Model* model, bool isSlopeGroundContact);

		/// @brief 現在の動作状態を取得
		/// @return 現在の動作状態
		Player::Motion GetCurrentMotion() const { return currentMotion_; }

		/// @brief Y方向速度を取得
		/// @return Y方向速度
		float GetVelocityY() const { return velocityY_; }

		/// @brief スライディング速度を取得
		/// @return スライディング速度
		Vector3 GetSlideVelocity() const { return slideVelocity_; }

		/// @brief ジャンプ時の水平初速を取得
		/// @return ジャンプ時の水平初速
		Vector3 GetJumpMoveVelocity() const { return jumpMoveVelocity_; }

		/// @brief 移動パラメータを取得
		/// @return 移動パラメータ
		Player::MovementParams& GetParams() { return movementParams_; }

	private:

		void Move(float deltaTime, Transform3D& transform, const Camera* camera, const MoveInput& input);

		void Jump(float deltaTime, Transform3D& transform, const MoveInput& input);

		/// @brief Slope上を移動しているときに足元を斜面へ追従
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		void ApplySlopeGroundSnap(float deltaTime, Transform3D& transform);

		/// @brief ジャンプ時に加えた水平初速を反映
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		void ApplyJumpMoveBoost(float deltaTime, Transform3D& transform);

		/// @brief 移動入力中のジャンプに水平初速を加える
		/// @param input Playerの移動操作入力
		void AddJumpMoveBoost(const MoveInput& input);

		/// @brief Crouching中のスライディング速度を更新
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		/// @param isCrouching Crouching入力が押されていればtrue
		/// @param isCrouchingStarted このフレームでCrouching入力が押され始めたらtrue
		/// @param moveDir 入力から計算した水平移動方向
		/// @param hasMoveInput 移動入力があればtrue
		void UpdateSliding(float deltaTime, Transform3D& transform, bool isCrouching, bool isCrouchingStarted, const Vector3& moveDir, bool hasMoveInput);

		/// @brief 現在接地しているSlopeの下り方向を取得
		/// @param outDownDirection 下り方向の出力先
		/// @return Slopeの下り方向を取得できればtrue
		bool TryGetSlopeDownDirection(Vector3& outDownDirection) const;

		/// @brief 水平スライディング速度に摩擦を適用
		/// @param deltaTime 1フレームの経過時間
		/// @param friction 減速量
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
		float remainingJumpCount_ = 0.0f;    // 残りジャンプ回数

		Player::MovementParams movementParams_; // 移動に関するパラメータ

		bool hasMoveInput_ = false; // 移動入力があるか

		float faceYaw_ = 0.0f; // Playerの水平な向き

		float modelGroundNormalFollowSpeed_ = 16.0f; // Model接地法線の補間速度
		Vector3 currentGroundNormal_ = { 0.0f, 1.0f, 0.0f }; // 補間中の接地法線

		Player::Motion currentMotion_ = Player::Motion::Idle;
	};

}