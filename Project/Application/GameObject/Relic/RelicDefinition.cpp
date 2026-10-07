#include "RelicDefinition.h"
#include <algorithm>
#include <cmath>

namespace Relic {

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
		return index < kStatCount ? flat_[index] : 0.0f;
	}

	float ModifierSnapshot::GetAdditiveRate(Stat stat) const {
		const std::size_t index = static_cast<std::size_t>(stat);
		return index < kStatCount ? additiveRate_[index] : 0.0f;
	}

}
