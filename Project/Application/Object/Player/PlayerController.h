#pragma once
#include "MathHeaders.h"

/// @brief Playerの移動操作入力を保持します。
struct PlayerMoveInput {
	Vector2 move = { 0.0f, 0.0f };   // 正規化済みの移動入力
	bool isJumpTriggered = false;    // ジャンプ入力が押された瞬間ならtrue
	bool isCrouching = false;        // しゃがみ入力中ならtrue
	bool isCrouchingStarted = false; // このフレームでしゃがみ入力が始まったらtrue
};

class PlayerController {
public:

	/// @brief Playerの操作入力を更新します。
	void Update();

	/// @brief 移動操作入力を取得します。
	/// @return 移動操作入力です。
	const PlayerMoveInput& GetMoveInput() const { return moveInput_; }

private:

	PlayerMoveInput moveInput_;
	bool wasCrouching_ = false; // 前フレームのしゃがみ入力状態
};
