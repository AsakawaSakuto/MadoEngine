#pragma once

namespace Enemy {
	namespace Data {

		/// @brief Enemyの現在ステータスを管理する構造体
		struct Status {
			float currentHealth = 100.0f; // 現在の体力
			float power = 1.0f;			  // Playerに与えるダメージ量
			float moveSpeed = 3.0f;		  // 移動速度
		};

		/// @brief Enemyの種類を表す列挙型
		enum class Type {
			Normal, // 通常の敵
			Speed,	// 速度が速い敵
		};

		/// @brief 特殊Enemy、倒された際にTypeの報酬を大量に落とす種類を表す列挙型
		enum class BonusType {
			None,  // 特になし
			Money, // お金 大量
			Exp,   // 経験値 大量
		};
	} // namespace Data
} // namespace Enemy
