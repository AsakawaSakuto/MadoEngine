#include "GamePlayTimerView.h"
#include ".SceneManager/SceneType.h"
#include "Audio/MyAudio.h"
#include "Render/Object/2d/Text/MyText.h"
#include <cmath>
#include <format>

namespace {
	constexpr const char* kTextObjectName = "GamePlayTimerText";
	constexpr const char* kOvertimeTextObjectName = "OverTimeText";
	constexpr const char* kOvertimeStartSoundKey = "OverTimeStart";
	constexpr float kOvertimeFadeDuration = 1.0f;
}

namespace UI::Game {

	void GamePlayTimerView::Initialize() {
		timerText_ = {};
		displayedSeconds_ = -1;
		isOvertime_ = false;
		ResetOvertimePresentation();
	}

	void GamePlayTimerView::Update(float remainingTime, float deltaTime) {

		// TimerTextの欠落や同一秒数による更新省略から独立して開始演出を進行
		UpdateOvertimePresentation(remainingTime, deltaTime);
		auto timerTextHandle = MyText::Find(kTextObjectName);
		MadoEngine::Text* timerText = MyText::TryGet(timerTextHandle);
		if (!timerText) {
			return;
		}

		// Text再生成時はEditor設定の色を取得し、同じ秒数でも新しいTextへ表示を反映
		if (timerText_ != timerTextHandle) {
			timerText_ = timerTextHandle;
			normalColor_ = timerText->GetColor();
			displayedSeconds_ = -1;
			isOvertime_ = false;
		}

		// 0秒の境界では表示秒数が同じでも文字色を切り替え、設定済みAlphaを維持
		const bool isOvertime = remainingTime <= 0.0f;
		if (isOvertime != isOvertime_) {
			Vector4 color = normalColor_;
			if (isOvertime) {
				color = { 1.0f, 0.0f, 0.0f, color.w };
			}
			color.w = timerText->GetColor().w;
			timerText->SetColor(color);
			isOvertime_ = isOvertime;
		}

		// 残り時間は切り上げ、超過時間は経過した整数秒だけを加算表示
		int displaySeconds = static_cast<int>(std::ceil(remainingTime));
		if (isOvertime) {
			displaySeconds = static_cast<int>(std::floor(-remainingTime));
		}

		// 表示秒数が変化したFrameだけTextを更新
		if (displaySeconds == displayedSeconds_) {
			return;
		}

		const int minutes = displaySeconds / 60;
		const int seconds = displaySeconds % 60;
		timerText->SetText(std::format("{:02}:{:02}", minutes, seconds));
		displayedSeconds_ = displaySeconds;
	}

	void GamePlayTimerView::Finalize() {

		// 次回プレイへ超過時の赤色を持ち越さないよう通常の文字色へ復元
		if (MadoEngine::Text* timerText = MyText::TryGet(timerText_)) {
			timerText->SetColor(normalColor_);
		}
		timerText_ = {};
		displayedSeconds_ = -1;
		isOvertime_ = false;
		ResetOvertimePresentation();
	}

	void GamePlayTimerView::UpdateOvertimePresentation(float remainingTime, float deltaTime) {
		if (remainingTime > 0.0f) {

			// 再プレイや時間の巻き戻しでは表示と通知済み状態を同時に解除
			ResetOvertimePresentation();
			return;
		}

		// Textの再生成では通知を再発火せず、開始Frameは透明度0から演出を開始
		if (!hasOvertimeStarted_) {
			hasOvertimeStarted_ = true;
			overtimeFadeTimer_.Start(kOvertimeFadeDuration);
			if (MyAudio::IsLoaded(kOvertimeStartSoundKey)) {
				MyAudio::Play(kOvertimeStartSoundKey);
			}
		} else {

			// ゲーム時間の停止中も実時間でフェードを完了し、終了後は透明度1を維持
			overtimeFadeTimer_.Update(deltaTime);
		}

		// Json再読込後も最新Textへ進行率を反映し、Editor設定のRGBを維持
		if (MadoEngine::Text* overtimeText = MyText::TryGet(MyText::Find(kOvertimeTextObjectName))) {
			Vector4 color = overtimeText->GetColor();
			color.w = overtimeFadeTimer_.GetProgress();
			overtimeText->SetColor(color);
			overtimeText->SetVisible(true);
		}
	}

	void GamePlayTimerView::ResetOvertimePresentation() {
		hasOvertimeStarted_ = false;
		overtimeFadeTimer_.Reset();

		// Scene開始直後や再プレイ時にEditor保存済みの表示状態が残ることを防止
		if (MadoEngine::Text* overtimeText = MyText::TryGet(MyText::Find(kOvertimeTextObjectName))) {
			Vector4 color = overtimeText->GetColor();
			color.w = 0.0f;
			overtimeText->SetColor(color);
			overtimeText->SetVisible(false);
		}
	}
}
