#pragma once
#include "GameObject/Map/MapLimit.h"
#include "MathHeaders.h"
#include <string>

namespace Enemy {

	/// @brief Enemyの地形判定と移動状態を管理するクラス
	class Movement {
	public:
		/// @brief 移動状態を初期化する
		void Initialize();

		/// @brief 目標座標へ向かう移動と重力を更新する
		/// @param deltaTime 前フレームからの経過時間
		/// @param targetPosition 移動目標のワールド座標
		/// @param moveSpeed 水平方向の移動速度
		/// @param transform 更新対象のTransform
		/// @return EnemyがMap内に存在していればtrueを返す
		bool Update(float deltaTime, const Vector3& targetPosition, float moveSpeed, Transform3D& transform);

		/// @brief Collider更新後に地形との接触状態を解決する
		/// @param movementColliderName 移動用Colliderの登録名
		/// @param transform 更新対象のTransform
		void ResolveAfterCollision(const std::string& movementColliderName, Transform3D& transform);

	private:
		/// @brief 目標へ向かう正規化済みの水平方向を取得する
		/// @param currentPosition 現在のワールド座標
		/// @param targetPosition 移動目標のワールド座標
		/// @return 正規化済みの水平方向
		Vector3 GetDirectionToTarget(const Vector3& currentPosition, const Vector3& targetPosition) const;

		/// @brief 側面で詰まったEnemyを少しずつ上へ補助する
		/// @param deltaTime 前フレームからの経過時間
		/// @param isGroundContact 通常地面に接地していればtrue
		/// @param isSlopeGroundContact 坂に接地していればtrue
		/// @param transform 更新対象のTransform
		void ApplySideClimbAssist(float deltaTime, bool isGroundContact, bool isSlopeGroundContact, Transform3D& transform);

		/// @brief 側面上昇補助の状態を初期化する
		/// @param currentPosition 現在のワールド座標
		void ResetSideClimbAssist(const Vector3& currentPosition);

		/// @brief 今回の移動が側面で止められたか判定する
		/// @param currentPosition Collider解決後のワールド座標
		/// @return 側面で前進量が落ちていればtrueを返す
		bool IsSideBlockedThisFrame(const Vector3& currentPosition) const;

		/// @brief 接地面の法線に合わせてTransformの回転を更新する
		/// @param movementColliderName 移動用Colliderの登録名
		/// @param isSlopeGroundContact 坂に接地していればtrue
		/// @param transform 更新対象のTransform
		void UpdateGroundRotation(const std::string& movementColliderName, bool isSlopeGroundContact, Transform3D& transform);

		MapLimit mapLimit_;
		float gravity_ = 30.0f;
		float velocityY_ = 0.0f;
		float lastDeltaTime_ = 0.0f;
		float sideClimbBaseY_ = 0.0f;
		float sideClimbAmount_ = 0.0f;
		float sideClimbCrestTimer_ = 0.0f;
		float faceYaw_ = 0.0f;
		float modelGroundNormalFollowSpeed_ = 16.0f;
		Vector3 lastMoveStartPosition_ = { 0.0f, 0.0f, 0.0f };
		Vector3 lastDesiredHorizontalMove_ = { 0.0f, 0.0f, 0.0f };
		Vector3 currentGroundNormal_ = { 0.0f, 1.0f, 0.0f };
		bool isGrounded_ = false;
		bool isSideClimbing_ = false;
	};
} // namespace Enemy
