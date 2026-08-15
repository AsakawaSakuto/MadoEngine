#pragma once
#include "../IGameObject.h"
#include "PlayerController.h"
#include "PlayerAnimationController.h"
#include "PlayerMovement.h"
#include "PlayerStatus.h"
#include "../Map/MapLimit.h"
#include "Render/Object/3d/EffectSequence/MyEffectSequence3d.h"

namespace Player {

	/// @brief Playerの基底クラス
	class Base : public IGameObject {
	public:

		/// @brief Playerを初期化
		/// @param spawnGroundPosition Playerを配置する地表座標
		void Initialize(const Vector3& spawnGroundPosition);

		/// @brief 入力と移動を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Collider更新後に接地状態と描画状態を解決
		void ResolveAfterCollision();

		/// @brief Playerのワールド座標を取得
		Vector3 GetPosition() const { return transform_.translate; }

		/// @brief Playerの描画Model座標を取得
		/// @return PlayerのModelワールド座標
		Vector3 GetModelPosition() const;

		void SetCamera(Camera* camera) { camera_ = camera; }

		/// @brief 所持金を加算
		/// @param amount 加算する所持金
		void AddMoney(int amount);

		/// @brief 所持金を消費
		/// @param amount 消費する所持金
		/// @return 消費できた場合はtrue
		bool TrySpendMoney(int amount);

		/// @brief 所持金で指定額を支払えるか判定
		/// @param amount 支払いに必要な所持金
		/// @return 支払い可能な場合はtrue
		bool CanAfford(int amount) const;

		/// @brief 経験値を加算
		/// @param amount 加算する経験値
		void AddExp(int amount);

		/// @brief PlayerのHPを減少
		/// @param damage 減らすHP量
		void TakeDamage(float damage);

		Status GetStatus() const { return status_; }

		/// @brief 現在のPlayerレベルを取得
		/// @return 現在のPlayerレベル
		int GetLevel() const { return status_.level; }

		/// @brief 壁登り可能な残り時間を取得
		/// @return 壁登り可能な残り時間
		float GetWallClimbRemainingTime() const { return movement_.GetWallClimbRemainingTime(); }

		/// @brief 壁登り可能な最大時間を取得
		/// @return 壁登り可能な最大時間
		float GetWallClimbMaxDuration() const { return movement_.GetWallClimbMaxDuration(); }

		/// @brief 壁登り時間ゲージの表示状態を取得
		/// @return ゲージを表示する場合はtrue
		bool IsWallClimbGaugeVisible() const { return movement_.IsWallClimbGaugeVisible(); }

		void DrawImGui();

	private:

		/// @brief 経験値が上限に達している場合にレベルアップ
		void ProcessLevelUp();

		/// @brief Player直下の地面へ影の描画座標を更新
		void UpdateShadowTransform();

		/// @brief Playerの移動イベントEffect Sequenceを足元で再生
		/// @param assetName 再生するEffect Sequenceアセット名
		void PlayMovementEffect(const std::string& assetName) const;

		/// @brief 一定間隔でPlayerのHPを自動回復
		/// @param deltaTime 前フレームからの経過時間
		void UpdateHealthRegeneration(float deltaTime);

		ColliderShape hitAABB_;
		ColliderShape expGetSphere_;
		ColliderShape attackRangeSphere_;

		Camera* camera_ = nullptr;

		Transform3D shadowTransform_;  // 影Transform
		MadoEngine::EffectSequence::MyEffectSequence3d landingMarker_; // 着地点Marker

		Status status_;                     // ステータス
		StatusMultiplier statusMultiplier_; // ステータスの倍率
		Controller controller_;             // 入力制御
		AnimationController animationController_; // 描画Animation制御
		Movement movement_;                 // 移動処理
		MoveInput lastMoveInput_;           // Collider解決時に使用する直近の移動入力
		float lastDeltaTime_ = 0.0f;        // Collider解決時に使用する直近の経過時間

		MapLimit mapLimit_; // Mapの制限範囲

		GamingColor gamingColor_; // ゲーミングカラー

		GameTimer regenerationTimer_; // HPの自動回復タイマー
	};
}
