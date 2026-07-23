#include "PlayerHealthGauge.h"
#include <format>

namespace {
	constexpr const char* kHealthTextObjectName = "PlayerHealthText";
}

namespace UI::Game {

	void PlayerHealthGauge::Initialize() {
		healthGauge_ = std::make_unique<Gauge>();
		healthGauge_->Initialize("PlayerHealthGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
		healthText_ = MyText::Create(
			kHealthTextObjectName,
			"HP : 0 / 0",
			SceneType::Game,
			MadoEngine::EditorManagementMode::EditorManaged,
			MadoEngine::Render::RenderLayer::UI);
	}

	void PlayerHealthGauge::Update(float currentHealth, float maxHealth) {
		if (healthGauge_) {
			healthGauge_->SetCurrentValue(currentHealth);
			healthGauge_->SetMaxValue(maxHealth);
		}

		if (healthText_) {
			healthText_->SetText(std::format("HP : {} / {}", currentHealth, maxHealth));
		}
	}

	void PlayerHealthGauge::Finalize() {
		MyText::Destroy(kHealthTextObjectName);
		healthText_ = nullptr;
	}

	void PlayerHealthGauge::DrawImGui() {
		if (healthGauge_) {
			healthGauge_->DrawImGui("HealthGauge");
		}
	}
}
