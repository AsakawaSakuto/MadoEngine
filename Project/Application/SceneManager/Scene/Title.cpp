#include "Title.h"
#include "SceneManager/SceneManager.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Title::Title()
	: sceneManager_(nullptr)
{
}

Title::~Title()
{
}

void Title::Initialize()
{
	Logger::Output("タイトルシーンを初期化しました", Logger::Level::Application);
}

void Title::Update()
{
	// スペースキーが押されたらゲームシーンに遷移
	if (MyInput::GetKeybord()->IsTrigger(DIK_SPACE))
	{
		Logger::Output("スペースキーが押されました - Gameシーンへ遷移", Logger::Level::Application);
		if (sceneManager_)
		{
			sceneManager_->ChangeScene("Game");
		}
	}
}

void Title::Draw()
{
	// タイトルシーンの描画処理
}

void Title::SetSceneManager(SceneManager* sceneManager)
{
	sceneManager_ = sceneManager;
}