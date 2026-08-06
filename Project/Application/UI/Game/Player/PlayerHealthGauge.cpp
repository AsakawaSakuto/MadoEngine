#include "PlayerHealthGauge.h"
#include <format>

namespace {
	constexpr const char* kHealthTextObjectName = "PlayerHealthText";
}

namespace UI::Game {

	void PlayerHealthGauge::Initialize() {
		healthGauge_ = std::make_unique<Gauge>();
		healthGauge_->Initialize("PlayerHealthGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
	}

	void PlayerHealthGauge::Update(float currentHealth, float maxHealth) {
		if (healthGauge_) {
			healthGauge_->SetCurrentValue(currentHealth);
			healthGauge_->SetMaxValue(maxHealth);
		}

		auto healthTextHandle = MyText::Find(kHealthTextObjectName);
		MadoEngine::Text* healthText = MyText::TryGet(healthTextHandle);
		if (healthText) {
			healthText->SetText(std::format("HP : {} / {}", static_cast<int>(currentHealth), static_cast<int>(maxHealth)));
		}
	}

	void PlayerHealthGauge::Finalize() {
		
	}

	void PlayerHealthGauge::DrawImGui() {
		if (healthGauge_) {
			healthGauge_->DrawImGui("HealthGauge");
		}
	}
}
