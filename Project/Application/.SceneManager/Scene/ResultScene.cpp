#include "ResultScene.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

ResultScene::ResultScene(CommonData& commonData)
	: commonData_(commonData) {}

ResultScene::~ResultScene() {}

void ResultScene::Initialize() {
	Logger::Output("リザルトシーンを初期化しました", Logger::Level::Application);
}

SceneType ResultScene::Update(float dt) {
	if (commonData_.GetSceneTransitionController().IsTransitioning()) {
		return SceneType::Result;
	}

	MadoEngine::InputDevice::Keyboard* keyboard = MyInput::GetKeyboard();
	MadoEngine::InputDevice::GamePad* gamePad = MyInput::GetGamePad();
	const bool isTitleTriggered =
		(keyboard && keyboard->IsTrigger(DIK_SPACE)) ||
		(gamePad && gamePad->IsTrigger(GAMEPAD_A));
	const bool isGameTriggered =
		(keyboard && keyboard->IsTrigger(DIK_Q)) ||
		(gamePad && gamePad->IsTrigger(GAMEPAD_X));

	// 同時入力時はGame再開よりTitleへ戻る操作を優先
	if (isTitleTriggered) {
		Logger::Output("Titleシーンへの遷移入力を受け付けました", Logger::Level::Application);
		return SceneType::Title;
	}

	if (isGameTriggered) {
		Logger::Output("Gameシーンへの遷移入力を受け付けました", Logger::Level::Application);
		return SceneType::Game;
	}

	return SceneType::Result;
}

void ResultScene::Draw() {
}

void ResultScene::DrawImGui() {
}

void ResultScene::Finalize() {
	Logger::Output("リザルトシーンの終了処理を実行しました", Logger::Level::Application);
}
