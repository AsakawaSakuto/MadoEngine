#include "Game.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Game::Game() {}

Game::~Game() {}

void Game::Initialize() {
	Logger::Output("ゲームシーンを初期化しました", Logger::Level::Application);
}

SceneType Game::Update(float dt) {
	// スペースキーが押されたらリザルトシーンに遷移
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE)) {
		Logger::Output("スペースキーが押されました - Resultシーンへ遷移", Logger::Level::Application);
		return SceneType::Result;
	}
	return SceneType::Game;
}

void Game::Draw() {
	// ゲームシーンの描画処理
}

void Game::DrawImGui() {
	// ゲームシーンのImGui描画処理
}

void Game::Finalize() {
	Logger::Output("ゲームシーンの終了処理を実行しました", Logger::Level::Application);
}