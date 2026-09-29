#include "Result.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Result::Result(CommonData& commonData)
	: commonData_(commonData) {}

Result::~Result() {}

void Result::Initialize() {
	Logger::Output("リザルトシーンを初期化しました", Logger::Level::Application);
}

SceneType Result::Update(float dt) {
	if (commonData_.GetSceneTransitionController().IsTransitioning()) {
		return SceneType::Result;
	}

	MadoEngine::InputDevice::Keybord* keyboard = MyInput::GetKeybord();
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

void Result::Draw() {
}

void Result::DrawImGui() {
}

void Result::Finalize() {
	Logger::Output("リザルトシーンの終了処理を実行しました", Logger::Level::Application);
}
