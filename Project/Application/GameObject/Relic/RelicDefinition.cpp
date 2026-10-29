#include "RelicDefinition.h"
#include <algorithm>
#include <cmath>

namespace Relic {

	float PeriodicEffect::GetIntervalSeconds(int stackCount) const {
		return std::max(kMinIntervalSeconds, intervalSeconds / static_cast<float>(std::max(1, stackCount)));
	}

	ModifierSnapshot::ModifierSnapshot() {
		Reset();
	}

	void ModifierSnapshot::Reset() {
		flat_.fill(0.0f);
		additiveRate_.fill(0.0f);
	}

	void ModifierSnapshot::Accumulate(const StatModifierEffect& effect, int stackCount) {
		if (stackCount <= 0 || !std::isfinite(effect.valuePerStack)) {
			return;
		}

		const std::size_t index = static_cast<std::size_t>(effect.stat);
		if (index >= kStatCount) {
			return;
		}

		// 同種レリックの効果を所持数に比例させて集約
		switch (effect.operation) {
		case ModifierOperation::AddFlat:
			flat_[index] += effect.valuePerStack * static_cast<float>(stackCount);
			break;
		case ModifierOperation::AddPercent:
			additiveRate_[index] += effect.valuePerStack * static_cast<float>(stackCount);
			break;
		}
	}

	float ModifierSnapshot::Resolve(Stat stat, float baseValue) const {
		const std::size_t index = static_cast<std::size_t>(stat);
		if (index >= kStatCount || !std::isfinite(baseValue)) {
			return baseValue;
		}

		return (baseValue + flat_[index]) * (1.0f + additiveRate_[index]);
	}

	float ModifierSnapshot::GetFlat(Stat stat) const {
		const std::size_t index = static_cast<std::size_t>(stat);
		if (index < kStatCount) {
			return flat_[index];
		} else {
			return 0.0f;
		}
	}

	float ModifierSnapshot::GetAdditiveRate(Stat stat) const {
		const std::size_t index = static_cast<std::size_t>(stat);
		if (index < kStatCount) {
			return additiveRate_[index];
		} else {
			return 0.0f;
		}
	}

}
