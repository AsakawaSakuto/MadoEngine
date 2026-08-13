#include "PlayerHealthGauge.h"
#include "Utility/Camera/Camera.h"
#include <format>

namespace {

	constexpr const char* kHealthTextObjectName = "PlayerHealthText";
	constexpr float kHealthGaugeHeightOffset = 2.2f;
	constexpr float kHealthGaugeCameraForwardOffset = -0.5f;
	constexpr Vector2 kHealthGauge3dSize = { 1.5f, 0.12f };

} // namespace

namespace UI::Game {

	void PlayerHealthGauge::Initialize() {
		healthGauge2d_ = std::make_unique<Gauge>();
		healthGauge2d_->Initialize("PlayerHealthGauge", SceneType::Game, MadoEngine::Render::RenderLayer::UI);

		// Jsonが存在しない場合に使用するPlayer頭上用の既定表示設定
		healthGauge3d_.SetTranslateOffset({ 0.0f, kHealthGaugeHeightOffset, 0.0f });
		healthGauge3d_.SetCameraTranslateOffset({ 0.0f, 0.0f, kHealthGaugeCameraForwardOffset });
		healthGauge3d_.SetSize(kHealthGauge3dSize);
		healthGauge3d_.SetBackgroundColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		healthGauge3d_.SetGaugeColor({ 0.0f, 1.0f, 0.0f, 1.0f });
		healthGauge3d_.SetDirection(Gauge3dDirection::Right);
		healthGauge3d_.Initialize(
			"PlayerHealthGauge3d",
			SceneType::Game,
			MadoEngine::Render::RenderLayer::World
		);
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
		healthGauge3d_.SetPosition(playerPosition);
		healthGauge3d_.Update(camera, currentHealth, maxHealth);

		auto healthTextHandle = MyText::Find(kHealthTextObjectName);
		MadoEngine::Text* healthText = MyText::TryGet(healthTextHandle);
		if (healthText) {
			healthText->SetText(std::format("HP : {} / {}", static_cast<int>(currentHealth), static_cast<int>(maxHealth)));
		}
	}

	void PlayerHealthGauge::Finalize() {
		healthGauge3d_.Finalize();
		healthGauge2d_.reset();
	}

	void PlayerHealthGauge::DrawImGui() {
		if (healthGauge2d_) {
			healthGauge2d_->DrawImGui("HealthGauge");
		}
		healthGauge3d_.DrawImGui("3D HPゲージ");
	}

} // namespace UI::Game
