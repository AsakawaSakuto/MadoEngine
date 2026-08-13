#include "PlayerExpGauge.h"

namespace UI::Game {

	void PlayerExpGauge::Initialize() {
		expGauge_ = std::make_unique<Gauge2d>();
		expGauge_->Initialize("PlayerExpGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);

		dopaGauge_ = MySprite::Create("PlayerExpDopaGauge", "DopaGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);
		if (Sprite* sprite = MySprite::TryGet(dopaGauge_)) {
			sprite->SetScale({ 160.0f, 32.0f });
		}
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
		if (Sprite* dopaGauge = MySprite::TryGet(dopaGauge_)) {
			if (isUpgrade) {

				// 強化選択中はUVを横へ流して待機状態を視覚化
				dopaGauge->SetVisible(true);

				uvOffsetX_ += uvSpeed_ * deltaTime;
				dopaGauge->SetUVTranslate(Vector2{ uvOffsetX_, 0.0f });
			} else {

				// 次回表示を同じ位相から始めるためUV進捗を初期化
				dopaGauge->SetVisible(false);
				uvOffsetX_ = 0.0f;
			}
		}
	}
}
