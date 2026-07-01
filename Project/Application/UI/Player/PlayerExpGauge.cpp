#include "PlayerExpGauge.h"

namespace Player {

	void ExpGauge::Initialize() {
		expGauge_ = std::make_unique<Gauge>();
		expGauge_->Initialize("PlayerExpGauge", SceneType::Test, MadoEngine::Render::RenderLayer::UI);
	}

	void ExpGauge::Update(float currentExp, float maxExp) {
		if (expGauge_) {
			expGauge_->SetCurrentValue(currentExp);
			expGauge_->SetMaxValue(maxExp);
		}
	}

	void ExpGauge::DrawImGui() {
		if (expGauge_) {
			expGauge_->DrawImGui("ExpGauge");
		}
	}
}