#pragma once
#include "Utility/Random.h"

namespace Combat {

	/// @brief Projectileの即死を抽選
	/// @param chance 即死率（%）
	/// @return 即死効果が発動する場合はtrue
	inline bool RollInstantKill(float chance) {
		constexpr float kGuaranteedChance = 100.0f;
		return chance > 0.0f &&
			(chance >= kGuaranteedChance || MyRand::GetFloat(0.0f, kGuaranteedChance) < chance);
	}

	/// @brief Projectileのクリティカルを抽選
	/// @param chance クリティカル率（%）
	/// @return クリティカルが発動する場合はtrue
	inline bool RollCritical(float chance) {
		constexpr int kRollMin = 1;
		constexpr int kRollMax = 100;
		return chance >= static_cast<float>(kRollMax) ||
			static_cast<float>(MyRand::GetInt(kRollMin, kRollMax)) <= chance;
	}

} // namespace Combat
