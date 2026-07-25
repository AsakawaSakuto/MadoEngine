#include "PlayerExpGauge.h"

namespace UI::Game {

	void PlayerExpGauge::Initialize() {
		expGauge_ = std::make_unique<Gauge>();
		expGauge_->Initialize("PlayerExpGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);

		dopaGauge_ = MySprite::Create("PlayerExpDopaGauge", "DopaGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
		dopaGauge_->SetScale({ 160.0f, 32.0f });
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

	void PlayerExpGauge::IsUpgrade(bool isUpgrade, float deltaTime) {
		if (dopaGauge_) {
			if (isUpgrade) {
				dopaGauge_->SetVisible(true);

				uvOffsetX_ += uvSpeed_ * deltaTime;
				dopaGauge_->SetUVTranslate(Vector2{ uvOffsetX_, 0.0f });
			} else {
				dopaGauge_->SetVisible(false);
				uvOffsetX_ = 0.0f;
			}
		}
	}
}
