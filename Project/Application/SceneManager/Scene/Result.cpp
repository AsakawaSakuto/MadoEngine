#include "Result.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Result::Result() {}

Result::~Result() {}

void Result::Initialize() {
	Logger::Output("リザルトシーンを初期化しました", Logger::Level::Application);
}

std::string Result::Update() {
	// スペースキーが押されたらタイトルシーンに遷移
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Titleシーンへ遷移", Logger::Level::Application);
		return "Title";
	}
	return "Result";
}

void Result::Finalize() {
	Logger::Output("リザルトシーンの終了処理を実行しました", Logger::Level::Application);
}

void Result::Draw() {
	// リザルトシーンの描画処理
}