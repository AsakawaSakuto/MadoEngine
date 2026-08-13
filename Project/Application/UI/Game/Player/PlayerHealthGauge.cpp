#include "PlayerHealthGauge.h"
#include "Utility/Camera/Camera.h"
#include <format>

namespace UI::Game {

	void PlayerHealthGauge::Initialize() {
		healthGauge2d_ = std::make_unique<Gauge2d>();
		healthGauge2d_->Initialize("PlayerHealthGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);

		healthGauge3d_ = std::make_unique<Gauge3d>();
		healthGauge3d_->Initialize("PlayerHealthGauge3d", SceneType::Game, MadoEngine::Render::RenderLayer::World);
	}

	void PlayerHealthGauge::Update(
		const Vector3& playerPosition,
		float currentHealth,
		float maxHealth,
		const Camera& camera) {
		if (healthGauge2d_) {
			healthGauge2d_->SetCurrentValue(currentHealth);
			healthGauge2d_->SetMaxValue(maxHealth);
		}

		// 同じHP情報から画面固定表示とPlayer追従表示を同期
		healthGauge3d_->SetPosition(playerPosition);
		healthGauge3d_->Update(camera, currentHealth, maxHealth);

		auto healthTextHandle = MyText::Find("PlayerHealthText");
		MadoEngine::Text* healthText = MyText::TryGet(healthTextHandle);
		if (healthText) {
			healthText->SetText(std::format("HP : {} / {}", static_cast<int>(currentHealth), static_cast<int>(maxHealth)));
		}
	}

	void PlayerHealthGauge::Finalize() {
		healthGauge3d_.reset();
		healthGauge2d_.reset();
	}

	void PlayerHealthGauge::DrawImGui() {
		
	}

} // namespace UI::Game
