#include "PlayerHealthGauge.h"
#include "Utility/Camera/Camera.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"

namespace UI::Game {

	void PlayerHealthGauge::Initialize() {
		auto& manager = MadoEngine::UIGroupManager::GetInstance();
		auto handle = manager.Find("PlayerHealthGauge", SceneType::Game);

		// Editorで復元済みの配置を優先し、保存データにない場合だけ新規生成
		if (!manager.TryGet(handle)) {
			handle = manager.CreateGauge("PlayerHealthGauge", SceneType::Game);
		}
		gaugeId_ = manager.GetId(handle);
		isVisible_ = true;

		healthGauge3d_ = std::make_unique<Gauge3d>();
		healthGauge3d_->Initialize("PlayerHealthGauge3d", SceneType::Game, MadoEngine::Render::RenderLayer::World);
	}

	void PlayerHealthGauge::SetVisible(bool isVisible) {
		isVisible_ = isVisible;

		// 個別の表示設定を維持した実行時の表示制御
		if (auto* gauge = TryGetGauge()) {
			gauge->SetVisible(isVisible_);
		}
		if (healthGauge3d_) {
			healthGauge3d_->SetVisible(isVisible);
		}
	}

	void PlayerHealthGauge::Update(
		const Vector3& playerPosition,
		float currentHealth,
		float maxHealth,
		const Camera& camera) {

		// 再読込やUndoで再生成されたゲージへ最新値と実行時の表示状態を再接続
		if (auto* gauge = TryGetGauge()) {
			gauge->SetVisible(isVisible_);
			gauge->Update(currentHealth, maxHealth);
		}

		// 同じHP情報から画面固定ゲージとPlayer追従ゲージを同期
		if (healthGauge3d_) {
			healthGauge3d_->SetPosition(playerPosition);
			healthGauge3d_->Update(camera, currentHealth, maxHealth);
		}
	}

	void PlayerHealthGauge::Finalize() {
		healthGauge3d_.reset();
		gaugeId_.clear();
	}

	Gauge2d* PlayerHealthGauge::TryGetGauge() const {
		auto& manager = MadoEngine::UIGroupManager::GetInstance();
		return manager.TryGetGauge(manager.FindById(gaugeId_));
	}

} // namespace UI::Game
