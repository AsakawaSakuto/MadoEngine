#pragma once
#include "Math/Vector3.h"
#include <cstdint>

namespace Combat {

	/// @brief ダメージの発生源種別
	enum class DamageSourceType : std::uint8_t {
		Weapon,
		Burn,
		Poison,
	};

	/// @brief ダメージ表示と武器戦績へ通知する共通イベント
	struct DamageEvent {
		Vector3 worldPosition = { 0.0f, 0.0f, 0.0f }; // ダメージ発生位置
		std::uint64_t sourceWeaponId = 0;              // 発生元の武器識別番号
		float appliedDamage = 0.0f;                    // 対象へ実際に適用されたダメージ量
		float displayDamage = 0.0f;                    // 画面表示用のダメージ量
		DamageSourceType sourceType = DamageSourceType::Weapon; // ダメージの発生源種別
		bool isCritical = false;                       // クリティカルダメージの場合はtrue
		bool wasKilled = false;                        // このダメージで対象を倒した場合はtrue
	};

} // namespace Combat
