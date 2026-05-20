#include "Game.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Game::Game() {}

Game::~Game() {}

void Game::Initialize() {
	Logger::Output("ゲームシーンを初期化しました", Logger::Level::Application);
}

std::string Game::Update() {
	// スペースキーが押されたらリザルトシーンに遷移
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Resultシーンへ遷移", Logger::Level::Application);
		return "Result";
	}
	return "Game";
}

void Game::Finalize() {
	Logger::Output("ゲームシーンの終了処理を実行しました", Logger::Level::Application);
}

void Game::Draw() {
	// ゲームシーンの描画処理
}