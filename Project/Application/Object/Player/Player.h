#pragma once
#include "../IGameObject.h"
#include "PlayerController.h"
#include "PlayerMovement.h"
#include "PlayerStatus.h"

class Player : public IGameObject {
public:

	void Initialize();

	void Update(float deltaTime) override;

	Vector3 GetPosition() const { return transform_.translate; }

	void SetCamera(Camera* camera) { camera_ = camera; }

	/// @brief 所持金を加算します。
	/// @param amount 加算する所持金です。
	void AddMoney(int amount);

	/// @brief 経験値を加算します。
	/// @param amount 加算する経験値です。
	void AddExp(int amount);
	
	void DrawImGui();

private:

	ColliderShape hitAABB_;

	Camera* camera_ = nullptr;

	PlayerStatus status_;                     // ステータス
	PlayerStatusMultiplier statusMultiplier_; // ステータスの倍率
	PlayerController controller_;             // 入力制御
	PlayerMovement movement_;                 // 移動処理
};
