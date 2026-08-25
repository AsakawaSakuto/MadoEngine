#pragma once

namespace Enemy {
	namespace Data {

		/// @brief Enemyの現在ステータスを管理する構造体
		struct Status {
			float currentHealth = 10.0f; // 現在の体力
			float power = 5.0f;			  // Playerに与えるダメージ量
			float moveSpeed = 3.0f;		  // 移動速度
		};

		/// @brief Enemyの種類を表す列挙型
		enum class Type {
			Normal, // 通常の敵
			Runner, // 低耐久で高速な敵
			Tank,   // 高耐久で低速な敵

			Boss,	// ボス敵
		};

		/// @brief Enemyへ付与する特殊属性の種類を表す列挙型
		enum class BonusType {
			None,  // 特になし
			Elite, // エリート敵
			Money, // お金 大量
			Exp,   // 経験値 大量
		};
	} // namespace Data
} // namespace Enemy
