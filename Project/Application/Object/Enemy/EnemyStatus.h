#pragma once

namespace Enemy {

	/// @brief Enemyの現在ステータスを管理する構造体
	struct Status {
		int currentHealth = 100; // 現在の体力
		int power = 1;          // Playerに与えるダメージ量
		float moveSpeed = 5.0f;  // 移動速度
	};
}