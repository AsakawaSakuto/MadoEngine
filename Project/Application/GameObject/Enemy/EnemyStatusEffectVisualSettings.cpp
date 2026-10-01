#include "EnemyStatusEffectVisualSettings.h"

namespace Enemy::StatusEffect {

	VisualSettings& VisualSettings::GetInstance() {
		static VisualSettings instance;
		return instance;
	}

	const VisualStyle& VisualSettings::Get(Type type) const {
		const std::size_t index = static_cast<std::size_t>(type);
		if (index >= styles_.size()) {
			return fallbackStyle_;
		}

		return styles_[index];
	}

	VisualStyle& VisualSettings::Edit(Type type) {
		const std::size_t index = static_cast<std::size_t>(type);
		if (index >= styles_.size()) {
			return fallbackStyle_;
		}

		return styles_[index];
	}

	VisualSettings::VisualSettings()
		: styles_{
			VisualStyle{ { 2.5f, 0.45f, 0.25f, 1.0f }, { 1.0f, 0.45f, 0.25f, 1.0f } },
			VisualStyle{ { 0.4f, 3.0f, 0.45f, 1.0f }, { 0.4f, 1.0f, 0.45f, 1.0f } },
			VisualStyle{ { 0.45f, 1.1f, 6.0f, 1.0f }, { 0.45f, 1.0f, 1.0f, 1.0f } },
		} {}

} // namespace Enemy::StatusEffect
