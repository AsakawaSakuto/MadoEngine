#include "SceneManager.h"
#include "IScene.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

SceneManager::SceneManager()
	: currentScene_(nullptr)
	, currentSceneType_(SceneType::Title) {}

SceneManager::~SceneManager() {}

void SceneManager::RegisterScene(SceneType type, CreatorFunc creator) {
	creators_[type] = std::move(creator);
	Logger::Output("シーンを登録しました: " + SceneTypeToString(type), Logger::Level::Debug);
}

void SceneManager::Initialize(SceneType initialScene) {
	Logger::Output("SceneManagerを初期化しました", Logger::Level::Application);
	ChangeScene(initialScene);
}

void SceneManager::Update() {
	MadoEngine::SpriteManager::GetInstance()->UpdateAll();

	ColliderManager::GetInstance()->Update();

	if (currentScene_) {
		SceneType next = currentScene_->Update();
		if (next != currentSceneType_) {
			ChangeScene(next);
		}
	}
}

void SceneManager::Draw() {
	MadoEngine::SpriteManager::GetInstance()->DrawAll(currentSceneType_);

	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::DrawImGui() {
	if (currentScene_) {
		currentScene_->DrawImGui();
	}
}

void SceneManager::ChangeScene(SceneType type) {
	auto it = creators_.find(type);
	if (it == creators_.end()) {
		Logger::Output("指定されたシーンは登録されていません: " + SceneTypeToString(type), Logger::Level::Error);
		assert(false && "未登録のSceneTypeが指定されました。SceneManager::RegisterScene()で事前登録してください。");
		return;
	}

	if (currentScene_) {
		currentScene_->Finalize();
		Logger::Output("旧シーンの終了処理を実行しました: " + SceneTypeToString(currentSceneType_), Logger::Level::Application);
	}

	currentScene_ = it->second();
	currentSceneType_ = type;
	currentScene_->Initialize();
	Logger::Output("シーン遷移を完了しました: " + SceneTypeToString(currentSceneType_), Logger::Level::Application);
}
