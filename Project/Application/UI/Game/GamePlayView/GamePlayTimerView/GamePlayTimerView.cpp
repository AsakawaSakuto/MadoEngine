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
		displayedRemainingSeconds_ = -1;
	}

	void GamePlayTimerView::Update(float remainingTime) {
		auto timerTextHandle = MyText::Find(kTextObjectName);
		MadoEngine::Text* timerText = MyText::TryGet(timerTextHandle);
		if (!timerText) {
			return;
		}

		const int remainingSeconds = static_cast<int>(std::ceil((std::max)(0.0f, remainingTime)));

		// 表示秒数が変化したFrameだけTextを更新
		if (remainingSeconds == displayedRemainingSeconds_) {
			return;
		}

		const int minutes = remainingSeconds / 60;
		const int seconds = remainingSeconds % 60;
		timerText->SetText(std::format("{:02}:{:02}", minutes, seconds));
		displayedRemainingSeconds_ = remainingSeconds;
	}

	void GamePlayTimerView::Finalize() {
		displayedRemainingSeconds_ = -1;
	}
}
