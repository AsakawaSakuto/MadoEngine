#include "Result.h"
#include "SceneManager/SceneManager.h"
#include "Input/MyInput.h"
#include "Utility/Logger/Logger.h"

Result::Result()
	: sceneManager_(nullptr)
{
}

Result::~Result()
{
}

void Result::Initialize()
{
	Logger::Output("リザルトシーンを初期化しました", Logger::Level::Application);
}

void Result::Update()
{
	// スペースキーが押されたらタイトルシーンに遷移
	if (Input::GetKeybord()->IsTrigger(DIK_SPACE))
	{
		Logger::Output("スペースキーが押されました - Titleシーンへ遷移", Logger::Level::Application);
		if (sceneManager_)
		{
			sceneManager_->ChangeScene("Title");
		}
	}
}

void Result::Draw()
{
	// リザルトシーンの描画処理
}

void Result::SetSceneManager(SceneManager* sceneManager)
{
	sceneManager_ = sceneManager;
}