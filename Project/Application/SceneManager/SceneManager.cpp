#include "SceneManager.h"
#include "IScene.h"
#include "Scene/Title.h"
#include "Scene/Game.h"
#include "Scene/Result.h"
#include "Utility/Logger/Logger.h"

SceneManager::SceneManager()
	: currentScene_(nullptr)
	, nextScene_(nullptr)
	, currentSceneName_("")
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::Initialize()
{
	Logger::Output("SceneManagerを初期化しました", Logger::Level::Application);

	// 最初のシーンをTitleに設定
	ChangeScene("Title");
}

void SceneManager::Update()
{
	// シーン遷移がある場合は実行
	if (nextScene_)
	{
		currentScene_.reset();
		currentScene_ = std::move(nextScene_);
		currentScene_->Initialize();
		Logger::Output("シーン遷移: " + currentSceneName_, Logger::Level::Application);
	}

	// 現在のシーンを更新
	if (currentScene_)
	{
		currentScene_->Update();
	}
}

void SceneManager::Draw()
{
	// 現在のシーンを描画
	if (currentScene_)
	{
		currentScene_->Draw();
	}
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	nextScene_ = CreateScene(sceneName);
	currentSceneName_ = sceneName;
}

std::unique_ptr<IScene> SceneManager::CreateScene(const std::string& sceneName)
{
	if (sceneName == "Title")
	{
		auto scene = std::make_unique<Title>();
		scene->SetSceneManager(this);
		return scene;
	}
	else if (sceneName == "Game")
	{
		auto scene = std::make_unique<Game>();
		scene->SetSceneManager(this);
		return scene;
	}
	else if (sceneName == "Result")
	{
		auto scene = std::make_unique<Result>();
		scene->SetSceneManager(this);
		return scene;
	}

	Logger::Output("不明なシーン名: " + sceneName, Logger::Level::Warning);
	return nullptr;
}