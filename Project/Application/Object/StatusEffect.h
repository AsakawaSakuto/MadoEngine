#pragma once

/// @brief ステータス異常の種類を表す列挙型
enum StatusEffectType {
    None,

    Burn,       // 火傷
    Poison,     // 毒
    Paralysis,  // 麻痺
    Freeze,     // 凍結
    Stun,       // 気絶
    Slow,       // 移動速度低下

    Count
};

/// @brief ステータス異常の情報を管理する構造体
struct StatusEffect {
	StatusEffectType type; // ステータス異常の種類
	float remainingTime;   // ステータス異常の残り時間
	int stackCount;        // ステータス異常の重ねがけ回数
};