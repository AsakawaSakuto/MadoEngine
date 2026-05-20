#include "SceneManager.h"
#include "IScene.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

SceneManager::SceneManager()
	: currentScene_(nullptr)
	, currentSceneName_("") {}

SceneManager::~SceneManager() {}

void SceneManager::RegisterScene(const std::string& sceneName, CreatorFunc creator) {
	creators_[sceneName] = std::move(creator);
	Logger::Output("シーンを登録しました: " + sceneName, Logger::Level::Debug);
}

void SceneManager::Initialize(const std::string& initialScene) {
	Logger::Output("SceneManagerを初期化しました", Logger::Level::Application);
	ChangeScene(initialScene);
}

void SceneManager::Update() {
	MadoEngine::SpriteManager::GetInstance()->UpdateAll();

	if (currentScene_) {
		std::string next = currentScene_->Update();
		if (!next.empty() && next != currentSceneName_) {
			ChangeScene(next);
		}
	}
}

void SceneManager::Draw() {
	MadoEngine::SpriteManager::GetInstance()->DrawAll(currentSceneName_);

	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::ChangeScene(const std::string& sceneName) {
	auto it = creators_.find(sceneName);
	if (it == creators_.end()) {
		Logger::Output("指定されたシーンは登録されていません: " + sceneName, Logger::Level::Error);
		assert(false && "未登録のシーン名が指定されました。SceneManager::RegisterScene()で事前登録してください。");
		return;
	}

	if (currentScene_) {
		currentScene_->Finalize();
		Logger::Output("旧シーンの終了処理を実行しました: " + currentSceneName_, Logger::Level::Application);
	}

	currentScene_ = it->second();
	currentSceneName_ = sceneName;
	currentScene_->Initialize();
	Logger::Output("シーン遷移を完了しました: " + currentSceneName_, Logger::Level::Application);
}