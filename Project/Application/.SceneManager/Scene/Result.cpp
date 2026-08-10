#include "Result.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Result::Result() {}

Result::~Result() {}

void Result::Initialize() {
	Logger::Output("リザルトシーンを初期化しました", Logger::Level::Application);
}

SceneType Result::Update(float dt) {

	// Result表示を維持しつつ決定入力だけをTitle遷移として受付
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Titleシーンへ遷移", Logger::Level::Application);
		return SceneType::Title;
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
