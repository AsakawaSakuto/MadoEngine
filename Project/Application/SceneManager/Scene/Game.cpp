#include "Game.h"
#include "SceneManager/SceneManager.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Game::Game()
	: sceneManager_(nullptr)
{
}

Game::~Game()
{
}

void Game::Initialize()
{
	Logger::Output("ゲームシーンを初期化しました", Logger::Level::Application);
}

void Game::Update()
{
	// スペースキーが押されたらリザルトシーンに遷移
	if (Input::GetKeybord()->IsTrigger(DIK_SPACE))
	{
		Logger::Output("スペースキーが押されました - Resultシーンへ遷移", Logger::Level::Application);
		if (sceneManager_)
		{
			sceneManager_->ChangeScene("Result");
		}
	}
}

void Game::Draw()
{
	// ゲームシーンの描画処理
}

void Game::SetSceneManager(SceneManager* sceneManager)
{
	sceneManager_ = sceneManager;
}