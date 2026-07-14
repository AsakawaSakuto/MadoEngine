#pragma once
#include "../IGameObject.h"
#include "PlayerController.h"
#include "PlayerMovement.h"
#include "PlayerStatus.h"
#include "../Map/MapLimit.h"

namespace Player {

	/// @brief Playerの基底クラス
	class Base : public IGameObject {
	public:

		void Initialize();

		void Update(float deltaTime) override;

		/// @brief Playerのワールド座標を取得する
		Vector3 GetPosition() const { return transform_.translate; }

		/// @brief Playerの描画Model座標を取得する
		/// @return PlayerのModelワールド座標
		Vector3 GetModelPosition() const;

		void SetCamera(Camera* camera) { camera_ = camera; }

		/// @brief 所持金を加算する
		/// @param amount 加算する所持金
		void AddMoney(int amount);

		/// @brief 経験値を加算
		/// @param amount 加算する経験値
		void AddExp(int amount);

		/// @brief PlayerのHPを減らす
		/// @param damage 減らすHP量
		void TakeDamage(float damage);

		Status GetStatus() const { return status_; }

		/// @brief 現在のPlayerレベルを取得
		/// @return 現在のPlayerレベル
		int GetLevel() const { return status_.level; }

		void DrawImGui();

	private:

		/// @brief 経験値が上限に達している場合にレベルアップ
		void ProcessLevelUp();

		/// @brief Player直下の地面へ影の描画座標を更新
		void UpdateShadowTransform();

		ColliderShape hitAABB_;
		ColliderShape expGetSphere_;

		Camera* camera_ = nullptr;

		Model* shadowModel_ = nullptr; // 影モデル
		Transform3D shadowTransform_;  // 影Transform

		Status status_;                     // ステータス
		StatusMultiplier statusMultiplier_; // ステータスの倍率
		Controller controller_;             // 入力制御
		Movement movement_;                 // 移動処理

		MapLimit mapLimit_; // Mapの制限範囲
	};
}
