#include "PlayerExpGauge.h"

namespace UI::Game {

	void PlayerExpGauge::Initialize() {
		expGauge_ = std::make_unique<Gauge>();
		expGauge_->Initialize("PlayerExpGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
	}

	void PlayerExpGauge::Update(float currentExp, float maxExp) {
		if (expGauge_) {
			expGauge_->SetCurrentValue(currentExp);
			expGauge_->SetMaxValue(maxExp);
		}
	}

	void PlayerExpGauge::DrawImGui() {
		if (expGauge_) {
			expGauge_->DrawImGui("ExpGauge");
		}
	}
}
