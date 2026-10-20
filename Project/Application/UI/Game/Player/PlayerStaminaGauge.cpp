#include "PlayerStaminaGauge.h"

namespace UI::Game {

	void PlayerStaminaGauge::Initialize() {
		staminaGauge_ = std::make_unique<Gauge2d>();
		staminaGauge_->Initialize("PlayerStaminaGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
		staminaGauge_->SetVisible(false);
	}

	void PlayerStaminaGauge::SetVisible(bool isVisible) {
		isVisible_ = isVisible;
		if (staminaGauge_) {
			staminaGauge_->SetVisible(isVisible_ && isClimbing_);
		}
	}

	void PlayerStaminaGauge::Update(float remainingTime, float maxDuration, bool isVisible) {
		isClimbing_ = isVisible;
		if (!staminaGauge_) {
			return;
		}

		staminaGauge_->SetVisible(isVisible_ && isClimbing_);
		staminaGauge_->Update(remainingTime, maxDuration);
	}

	void PlayerStaminaGauge::Finalize() {
		staminaGauge_.reset();
	}

	void PlayerStaminaGauge::DrawImGui() {
		if (staminaGauge_) {
			staminaGauge_->DrawImGui("PlayerStaminaGauge");
		}
	}

} // namespace UI::Game
