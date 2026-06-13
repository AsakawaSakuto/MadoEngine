#pragma once


struct PlayerStatus {
	int health = 100;      // プレイヤーの体力
};

/// @brief プレイヤーの動作状態を表す列挙型
enum class PlayerMotion {
	Idle,      // 何もしていない状態
	Walk,      // 歩いている状態 & 走ってる状態
	Crouching, // しゃがんでいる状態
	Jump,	   // ジャンプ中の状態
	
	Count
};