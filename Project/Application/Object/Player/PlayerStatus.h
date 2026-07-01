#pragma once

namespace Player {

	/// @brief プレイヤーの現在ステータスを管理する構造体
	struct Status {
		int maxHealth = 100; // 最大体力
		int currentHealth = 100; // 現在の体力

		int maxShield = 0; // 最大シールド
		int currentShield = 0; // 現在のシールド

		int level = 1; // レベル
		int currentExp = 0; // 現在の経験値
		int expToNextLevel = 100; // 次のレベルまでの経験値

		int currentMoney = 0; // 所持金
	};

	/// @brief プレイヤーの獲得量や与ダメージの倍率を管理する構造体
	struct StatusMultiplier {
		float money = 1.0f; // お金の倍率
		float exp = 1.0f; // 経験値の倍率
		float damage = 1.0f; // ダメージの倍率
		float movementSpeed = 1.0f; // 移動速度の倍率
	};

	/// @brief プレイヤーの移動に関するパラメータを管理する構造体
	struct MovementParams {
		float moveSpeed_ = 5.0f; // 歩行速度
		float jumpPower_ = 10.0f; // ジャンプの初速
		float gravity_ = 20.0f; // 重力加速度
		float slideStartSpeed_ = 7.5f; // スライディング開始時の水平速度
		float slideSteerRate_ = 5.0f; // スライディング中の旋回速度
		float slopeSlideAcceleration_ = 30.0f; // Slope上でのスライディング加速度
		float maxSlideSpeed_ = 25.0f; // スライディングの最大速度
		float slideFriction_ = 2.0f; // スライディング中の摩擦
		float jumpMoveBoostSpeed_ = 3.0f; // ジャンプ時の水平初速
		int   jumpCount_ = 10; // ジャンプ可能回数
	};

	/// @brief プレイヤーの動作状態を表す列挙型
	enum class Motion {
		Idle,      // 何もしていない状態
		Walk,      // 歩いている状態 & 走ってる状態
		Crouching, // しゃがんでいる状態
		Jump,	   // ジャンプ中の状態

		Count
	};

	/// @brief PlayerMotionを表示用文字列へ変換する
	/// @param motion 変換するPlayerMotion
	/// @return 表示用文字列
	inline const char* ToMotionText(Player::Motion motion) {
		switch (motion) {
		case Player::Motion::Idle:
			return "Idle";
		case Player::Motion::Walk:
			return "Walk";
		case Player::Motion::Crouching:
			return "Crouching";
		case Player::Motion::Jump:
			return "Jump";
		default:
			return "Unknown";
		}
	}

}