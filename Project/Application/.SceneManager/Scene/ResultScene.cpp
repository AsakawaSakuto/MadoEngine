#include "ResultScene.h"
#include "Audio/MyAudio.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

ResultScene::ResultScene(CommonData& commonData)
	: commonData_(commonData) {}

ResultScene::~ResultScene() {}

void ResultScene::Initialize() {
	isScoreVisible_ = false;
	const System::PlayLogData& result = commonData_.GetLastPlayResult();
	const System::ScoreRanking& ranking = commonData_.GetScoreRanking();
	std::optional<std::size_t> currentRank;

	// ランキング対象のプレイ結果だけに今回の順位を渡し、中断後の古い記録の強調を防止
	if (result.endReason == System::PlayEndReason::Death) {
		currentRank = ranking.GetLastRecordedRank();
	}
	weaponStatsUI_.Initialize(result);
	scoreUI_.Initialize(result, ranking.GetScores(), currentRank);
	Logger::Output("リザルトシーンを初期化しました", Logger::Level::Application);
}

SceneType ResultScene::Update(float dt) {

	// Sceneの遷移演出中に持ち込まれた決定入力でページを飛ばさないガード
	if (commonData_.GetSceneTransitionController().IsTransitioning()) {
		return SceneType::Result;
	}

	// スコア画面だけに経過時間を渡し、武器戦績の確認中は移動と加算を停止
	if (isScoreVisible_) {
		scoreUI_.Update(dt);
	}

	MadoEngine::InputDevice::Keyboard* keyboard = MyInput::GetKeyboard();
	MadoEngine::InputDevice::GamePad* gamePad = MyInput::GetGamePad();
	const bool isConfirmTriggered =
		(keyboard && (keyboard->IsTrigger(DIK_A) || keyboard->IsTrigger(DIK_SPACE))) ||
		(gamePad && gamePad->IsTrigger(GAMEPAD_A));
	const bool isGameTriggered =
		(keyboard && keyboard->IsTrigger(DIK_Q)) ||
		(gamePad && gamePad->IsTrigger(GAMEPAD_X));

	// 最初の決定入力は一覧からスコアへの切替だけに消費し、同じ入力でTitleへ進む状態を防止
	if (isConfirmTriggered) {
		MyAudio::Play("Confirm");
		if (!isScoreVisible_) {
			ShowScore();
			return SceneType::Result;
		}

		// 演出中の決定入力は即時完了に消費し、最終値を確認してからTitleへ移行
		if (!scoreUI_.IsPresentationFinished()) {
			scoreUI_.CompletePresentation();
			return SceneType::Result;
		}
		return SceneType::Title;
	}

	// 武器ごとの戦績確認中はリトライを受け付けず、スコア画面だけで再開始を許可
	if (isScoreVisible_ && isGameTriggered) {
		scoreUI_.StopPresentationAudio();
		MyAudio::Play("Confirm");
		return SceneType::Game;
	}

	return SceneType::Result;
}

void ResultScene::ShowScore() {

	// 武器と状態異常の全表示をまとめて無効化し、スコア画面へ一方向に遷移
	weaponStatsUI_.SetVisible(false);
	scoreUI_.SetVisible(true);
	isScoreVisible_ = true;
}

void ResultScene::Draw() {
}

void ResultScene::DrawImGui() {
}

void ResultScene::Finalize() {
	weaponStatsUI_.Finalize();
	scoreUI_.Finalize();
	Logger::Output("リザルトシーンの終了処理を実行しました", Logger::Level::Application);
}
