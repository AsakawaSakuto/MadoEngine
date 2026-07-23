#include "GamePlayTimerView.h"
#include ".SceneManager/SceneType.h"
#include "Render/Object/2d/Text/MyText.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace {
	constexpr const char* kTextObjectName = "GamePlayTimerText";
}

namespace UI::Game {

	void GamePlayTimerView::Initialize() {
		timerText_ = MyText::Create(
			kTextObjectName,
			"Time : 00:00",
			SceneType::Game,
			MadoEngine::EditorManagementMode::EditorManaged,
			MadoEngine::Render::RenderLayer::UI);
		displayedRemainingSeconds_ = -1;
	}

	void GamePlayTimerView::Update(float remainingTime) {
		if (!timerText_) {
			return;
		}

		const int remainingSeconds = static_cast<int>(std::ceil((std::max)(0.0f, remainingTime)));
		if (remainingSeconds == displayedRemainingSeconds_) {
			return;
		}

		const int minutes = remainingSeconds / 60;
		const int seconds = remainingSeconds % 60;
		timerText_->SetText(std::format("Time : {:02}:{:02}", minutes, seconds));
		displayedRemainingSeconds_ = remainingSeconds;
	}

	void GamePlayTimerView::Finalize() {
		MyText::Destroy(kTextObjectName);
		timerText_ = nullptr;
		displayedRemainingSeconds_ = -1;
	}
}
