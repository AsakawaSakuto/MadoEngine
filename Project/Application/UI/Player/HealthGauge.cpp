#include "HealthGauge.h"

namespace Player {

	void HealthGauge::Initialize() {
		healthGauge_ = std::make_unique<Gauge>();
		healthGauge_->Initialize("HealthGauge", SceneType::Test, MadoEngine::Render::RenderLayer::UI);
	}

	void HealthGauge::Update(float currentHealth, float maxHealth) {
		if (healthGauge_) {
			healthGauge_->SetCurrentValue(currentHealth);
			healthGauge_->SetMaxValue(maxHealth);
		}
	}

	void HealthGauge::DrawImGui() {
		if (healthGauge_) {
			healthGauge_->DrawImGui("HealthGauge");
		}
	}
}