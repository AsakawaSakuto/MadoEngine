#include "Title.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Title::Title() {}

Title::~Title() {}

void Title::Initialize() {
	Logger::Output("タイトルシーンを初期化しました", Logger::Level::Application);
}

SceneType Title::Update() {
	// スペースキーが押されたらゲームシーンに遷移
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Gameシーンへ遷移", Logger::Level::Application);
		return SceneType::Game;
	}
	return SceneType::Title;
}

void Title::Draw() {
	// タイトルシーンの描画処理
}

void Title::DrawImGui() {
	// タイトルシーンのImGui描画処理
}

void Title::Finalize() {
	Logger::Output("タイトルシーンの終了処理を実行しました", Logger::Level::Application);
}
