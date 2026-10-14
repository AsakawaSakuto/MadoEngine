#pragma once

/// @brief 成立したMapインタラクトの種別
enum class MapInteractionType {
	Other,
	PaidChest,
	FreeChest,
	ExpJar,
	MoneyJar,
	Karma,
};

/// @brief 成功したインタラクトの結果通知
struct MapInteractionEvent {
	MapInteractionType type = MapInteractionType::Other;
	int paidMoney = 0;
};
