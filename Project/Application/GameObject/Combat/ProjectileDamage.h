#pragma once
#include "Utility/Random.h"
#include <cmath>
#include <limits>

namespace Combat {

	/// @brief Projectileの即死を抽選
	/// @param chance 即死率（%）
	/// @return 即死効果が発動する場合はtrue
	inline bool RollInstantKill(float chance) {
		constexpr float kGuaranteedChance = 100.0f;
		return chance > 0.0f &&
			(chance >= kGuaranteedChance || MyRand::GetFloat(0.0f, kGuaranteedChance) < chance);
	}

	/// @brief Projectileの通常クリティカルと多重クリティカルの合計回数を抽選
	/// @param chance クリティカル率（%）
	/// @return 未発動の場合は0、通常クリティカルは1、多重クリティカルは2以上
	inline int RollCriticalCount(float chance) {
		if (!std::isfinite(chance) || chance <= 0.0f) {
			return 0;
		}

		// 100%ごとの確定回数を一括計算し、高いクリティカル率でも抽選回数を一定に維持
		constexpr float kGuaranteedChance = 100.0f;
		const double guaranteedCount = std::floor(static_cast<double>(chance) / kGuaranteedChance);
		if (guaranteedCount >= static_cast<double>((std::numeric_limits<int>::max)())) {

			// 極端な強化値による整数変換と追加抽選時のオーバーフローを防止
			return (std::numeric_limits<int>::max)();
		}

		int criticalCount = static_cast<int>(guaranteedCount);
		const float remainingChance = std::fmod(chance, kGuaranteedChance);

		// 100%未満の端数だけを抽選し、小数のクリティカル率も確率へ反映
		if (remainingChance > 0.0f && MyRand::GetFloat(0.0f, kGuaranteedChance) < remainingChance) {
			++criticalCount;
		}
		return criticalCount;
	}

	/// @brief 通常クリティカルと多重クリティカルのダメージ倍率を計算
	/// @param baseMultiplier 元のクリティカルダメージ倍率
	/// @param criticalCount 通常クリティカルを含む合計回数
	/// @return 未発動の場合は1、多重1回ごとに元の倍率の半分を加算した倍率
	inline float CalculateCriticalDamageMultiplier(float baseMultiplier, int criticalCount) {
		if (criticalCount <= 0) {
			return 1.0f;
		}

		// 最初の1回を通常クリティカルとし、追加分だけに元の倍率の半分を加算
		const int multiCriticalCount = criticalCount - 1;
		return baseMultiplier + (baseMultiplier / 2.0f) * static_cast<float>(multiCriticalCount);
	}

} // namespace Combat
