#pragma once

namespace Enemy {

	/// @brief Enemyの現在ステータスを管理する構造体
	struct Status {
		int currentHealth = 100; // 現在の体力
		int power = 1;           // Playerに与えるダメージ量
		float moveSpeed = 5.0f;  // 移動速度
	};

	/// @brief Enemyの種類を表す列挙型
	enum class Type {
		Normal, // 通常の敵
		Speed,  // 速度が速い敵

		Count
	};

	/// @brief 特殊Enemy、倒された際にTypeの報酬を大量に落とす種類を表す列挙型
	enum class BonusType {
		None,  // 特になし
		Money, // お金 大量
		Exp,   // 経験値 大量

		Count
	};
}