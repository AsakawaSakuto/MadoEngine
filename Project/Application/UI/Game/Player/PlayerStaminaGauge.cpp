#include "PlayerStaminaGauge.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"

namespace UI::Game {

	void PlayerStaminaGauge::Initialize() {
		auto& manager = MadoEngine::UIGroupManager::GetInstance();
		auto handle = manager.Find("PlayerStaminaGauge", SceneType::Game);

		// 保存済みのレイアウトを維持し、未登録の場合だけ既定のゲージを生成
		if (!manager.TryGet(handle)) {
			handle = manager.CreateGauge("PlayerStaminaGauge", SceneType::Game);
		}
		gaugeId_ = manager.GetId(handle);
		isVisible_ = true;
		isClimbing_ = false;
		if (auto* gauge = TryGetGauge()) {
			gauge->SetVisible(false);
		}
	}

	void PlayerStaminaGauge::SetVisible(bool isVisible) {
		isVisible_ = isVisible;

		// HUD全体の表示許可と壁登り中の条件を合成
		if (auto* gauge = TryGetGauge()) {
			gauge->SetVisible(isVisible_ && isClimbing_);
		}
	}

	void PlayerStaminaGauge::Update(float remainingTime, float maxDuration, bool isVisible) {
		isClimbing_ = isVisible;
		auto* gauge = TryGetGauge();
		if (!gauge) {
			return;
		}

		// Editorでの非表示とは独立した壁登り中の表示条件
		gauge->SetVisible(isVisible_ && isClimbing_);
		gauge->Update(remainingTime, maxDuration);
	}

	void PlayerStaminaGauge::Finalize() {
		gaugeId_.clear();
	}

	Gauge2d* PlayerStaminaGauge::TryGetGauge() const {
		auto& manager = MadoEngine::UIGroupManager::GetInstance();
		return manager.TryGetGauge(manager.FindById(gaugeId_));
	}

} // namespace UI::Game
