#pragma once
#include "../IGameObject.h"
#include "PlayerController.h"
#include "PlayerStatus.h"
#include "Utility/Parameter/DebugParameter.h"

namespace Player {

	/// @brief Playerの移動処理を担当するクラス
	class Movement {
	public:
		/// @brief 移動状態と調整値の既定構築
		Movement() = default;
		Movement(const Movement&) = delete;
		Movement& operator=(const Movement&) = delete;
		Movement(Movement&&) = delete;
		Movement& operator=(Movement&&) = delete;

		/// @brief 共通Inspectorへ移動設定を登録して初期値を反映
		/// @param parameterStore Movementより長く保持する設定値管理
		void RegisterDebugParameters(MadoEngine::ParameterStore& parameterStore);

		/// @brief 移動状態を初期化
		void Initialize();

		/// @brief 入力と重力をもとに移動を更新
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		/// @param camera 移動方向の基準にするCamera
		/// @param input Playerの移動操作入力
		void Update(float deltaTime, Transform3D& transform, const Camera* camera, const MoveInput& input);

		/// @brief 水平方向のノックバック衝撃を追加
		/// @param direction Playerを押し出す方向
		/// @param power ノックバック距離
		void ApplyKnockback(const Vector3& direction, float power);

		/// @brief 前フレーム末尾のCollider解決結果から水平移動の阻害状態を確定
		/// @param resolvedPosition Collider解決後のPlayer座標
		void CaptureCollisionResult(const Vector3& resolvedPosition);

		/// @brief 接地状態を移動処理へ反映
		/// @param isGroundContact 通常地面に接地していればtrue
		/// @param isSlopeGroundContact 坂に接地していればtrue
		/// @param input Playerの移動操作入力
		void SetGroundContact(bool isGroundContact, bool isSlopeGroundContact, const MoveInput& input);

		/// @brief Collider解決後の移動量からBlock側面への壁上りを更新
		/// @param deltaTime 1フレームの経過時間
		/// @param input Playerの移動操作入力
		/// @param transform 更新対象のTransform
		void UpdateWallClimb(float deltaTime, const MoveInput& input, Transform3D& transform);

		/// @brief PlayerのModel座標と回転を接地状態に合わせて更新
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		/// @param model 更新対象のModel
		/// @param isSlopeGroundContact 坂に接地していればtrue
		/// @param modelYawOffset Modelの正面軸を補正するYaw角度
		void UpdateModelTransform(float deltaTime, Transform3D& transform, Model* model, bool isSlopeGroundContact, float modelYawOffset);

		/// @brief 現在の動作状態を取得
		/// @return 現在の動作状態
		Player::Motion GetCurrentMotion() const { return currentMotion_; }

		/// @brief Playerが向いている水平Yaw角度を取得
		/// @return Z軸正方向を0とする水平Yaw角度（ラジアン）
		float GetFacingYaw() const { return faceYaw_; }

		/// @brief Playerの描画姿勢に適用している補間済み上方向を取得
		/// @return 正規化済みの上方向
		Vector3 GetUpDirection() const { return currentGroundNormal_; }

		/// @brief Y方向速度を取得
		/// @return Y方向速度
		float GetVelocityY() const { return velocityY_; }

		/// @brief Playerが接地中か判定
		/// @return 接地中の場合はtrue
		bool IsGrounded() const { return isGrounded_; }

		/// @brief このフレームにJumpが成立したか判定
		/// @return Jumpが成立した場合はtrue
		bool WasJumpStartedThisFrame() const { return jumpStartedThisFrame_; }

		/// @brief このフレームに着地したか判定
		/// @return 着地した場合はtrue
		bool WasLandedThisFrame() const { return landedThisFrame_; }

		/// @brief Block側面を上昇中か判定
		/// @return 壁上り中の場合はtrue
		bool IsWallClimbing() const { return isWallClimbing_; }

		/// @brief 着地後からの壁登り経過時間を取得
		/// @return 壁登り経過時間
		float GetWallClimbElapsedTime() const { return wallClimbElapsedTime_; }

		/// @brief 壁登り可能な残り時間を取得
		/// @return 壁登り可能な残り時間
		float GetWallClimbRemainingTime() const;

		/// @brief 壁登り可能な最大時間を取得
		/// @return 壁登り可能な最大時間
		float GetWallClimbMaxDuration() const;

		/// @brief 壁登り時間ゲージの表示状態を取得
		/// @return ゲージを表示する場合はtrue
		bool IsWallClimbGaugeVisible() const { return hasWallClimbStartedSinceLanding_ && !isGrounded_; }

		/// @brief スライディング速度を取得
		/// @return スライディング速度
		Vector3 GetSlideVelocity() const { return slideVelocity_; }

		/// @brief ジャンプ時の水平初速を取得
		/// @return ジャンプ時の水平初速
		Vector3 GetJumpMoveVelocity() const { return jumpMoveVelocity_; }

		/// @brief ノックバック速度を取得
		/// @return 現在の水平ノックバック速度
		Vector3 GetKnockbackVelocity() const { return knockbackVelocity_; }

		/// @brief 歩行速度へ適用する倍率を設定
		/// @param multiplier 1.0を基準とする速度倍率
		void SetMoveSpeedMultiplier(float multiplier);

		/// @brief レリック由来の追加ジャンプ回数を設定
		/// @param additionalJumpCount 追加するジャンプ回数
		void SetAdditionalJumpCount(int additionalJumpCount);

		/// @brief 移動パラメータを取得
		/// @return 移動パラメータ
		Player::MovementParams& GetParams() { return movementParams_; }

	private:

		/// @brief Camera基準の移動入力と水平初速による移動を更新
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		/// @param camera 移動方向の基準Camera
		/// @param input Playerの移動操作入力
		void Move(float deltaTime, Transform3D& transform, const Camera* camera, const MoveInput& input);

		/// @brief ジャンプ入力と重力による上下移動を更新
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		/// @param input Playerの移動操作入力
		void Jump(float deltaTime, Transform3D& transform, const MoveInput& input);

		/// @brief Slope上を移動しているときに足元を斜面へ追従
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		void ApplySlopeGroundSnap(float deltaTime, Transform3D& transform);

		/// @brief ジャンプ時に加えた水平初速を反映
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		void ApplyJumpMoveBoost(float deltaTime, Transform3D& transform);

		/// @brief 着地後の受付時間と受付終了後の水平減速を更新
		/// @param deltaTime 1フレームの経過時間
		void UpdateBunnyHopState(float deltaTime);

		/// @brief 着地直後の再Jumpへ進行方向の加速を適用
		/// @param input Playerの移動操作入力
		/// @return バニーホップが成立する場合はtrue
		bool TryAddBunnyHopBoost(const MoveInput& input);

		/// @brief バニーホップの水平初速を維持した入力方向への旋回
		/// @param deltaTime 1フレームの経過時間
		/// @param moveDir Camera基準の入力を変換したワールド空間の水平移動方向
		void UpdateBunnyHopSteering(float deltaTime, const Vector3& moveDir);

		/// @brief バニーホップ継続中の歩行と水平初速の合算速度を制限
		/// @param moveVelocity 移動入力による水平速度
		void ClampBunnyHopSpeed(const Vector3& moveVelocity);

		/// @brief ジャンプ由来の水平初速へ方向を維持した摩擦を適用
		/// @param deltaTime 減速対象の経過時間
		/// @param friction 1秒あたりの減速量
		void ApplyJumpMoveFriction(float deltaTime, float friction);

		/// @brief 移動入力中のジャンプに水平初速を加算
		/// @param input Playerの移動操作入力
		void AddJumpMoveBoost(const MoveInput& input);

		/// @brief 水平ノックバック速度を移動へ反映
		/// @param deltaTime 1フレームの経過時間
		/// @param transform 更新対象のTransform
		void UpdateKnockback(float deltaTime, Transform3D& transform);

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

		/// @brief Collider解決によって希望する水平移動がBlock側面で阻害されたか判定
		/// @param currentPosition Collider解決後のPlayer座標
		/// @return Block側面で前進を阻害された場合はtrue
		bool IsHorizontalMoveBlocked(const Vector3& currentPosition) const;

		/// @brief 壁登り中の向きを基準に背面方向へのJumpを開始
		void StartWallJump();

		/// @brief 壁登り時間切れによる強制Jumpを開始
		void StartWallClimbTimeoutJump();

		/// @brief 基礎値と外部補正から最大ジャンプ回数を取得
		/// @return 0以上の最大ジャンプ回数
		float GetMaxJumpCount() const;

	private:

		Vector3 slideVelocity_ = { 0.0f, 0.0f, 0.0f };
		Vector3 jumpMoveVelocity_ = { 0.0f, 0.0f, 0.0f };
		Vector3 knockbackVelocity_ = { 0.0f, 0.0f, 0.0f };
		Vector3 lastMoveDirection_ = { 0.0f, 0.0f, 0.0f };
		Vector3 lastMoveStartPosition_ = { 0.0f, 0.0f, 0.0f };
		Vector3 lastAttemptedHorizontalMove_ = { 0.0f, 0.0f, 0.0f };

		float velocityY_ = 0.0f;
		bool isGrounded_ = false;
		bool jumpStartedThisFrame_ = false;
		bool landedThisFrame_ = false;
		bool hasWallClimbInput_ = false;
		bool wasHorizontalMoveBlocked_ = false;
		bool isWallClimbing_ = false;
		bool hasWallClimbStartedSinceLanding_ = false;
		bool wasGroundContact_ = true;
		bool isBaseGroundContact_ = false;
		bool hasBunnyHopLanding_ = false;
		bool isBunnyHopMomentumActive_ = false;

		float groundY_ = 0.0f;               // 接地している地面のY座標
		float slopeSnapDistance_ = 1.0f;     // Slopeに足が届いているとみなす距離
		float jumpMoveBoostFriction_ = 0.0f; // ジャンプ時の水平初速に対する摩擦
		float slideReleaseFriction_ = 10.0f; // スライディング解除時の摩擦
		float remainingJumpCount_ = 0.0f;    // 残りジャンプ回数
		float wallClimbElapsedTime_ = 0.0f;  // 着地後から累積した壁登り時間
		float timeSinceLanding_ = 0.0f;     // バニーホップ判定用の着地後経過時間
		float moveSpeedMultiplier_ = 1.0f;   // 外部効果による歩行速度倍率
		int additionalJumpCount_ = 0;        // 外部効果による追加ジャンプ回数

		Player::MovementParams movementParams_; // 移動に関するパラメータ

		bool hasMoveInput_ = false; // 移動入力があるか

		float faceYaw_ = 0.0f; // Playerの水平な向き

		float modelGroundNormalFollowSpeed_ = 16.0f; // Model接地法線の補間速度
		Vector3 currentGroundNormal_ = { 0.0f, 1.0f, 0.0f }; // 補間中の接地法線

		Player::Motion currentMotion_ = Player::Motion::Idle;
		EaseType debugEaseType_ = EaseType::Linear; // 選択と保存の確認用、移動処理では未使用
		std::unique_ptr<MadoEngine::DebugParameter> debugParameter_;
	};

}
