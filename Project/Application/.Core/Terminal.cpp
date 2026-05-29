#include "Terminal.h"

Terminal::Terminal(HINSTANCE hInstance)
{
	execution_ = std::make_unique<MadoEngine::Execution>();
	execution_->Initialize(hInstance);
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->RegisterScene(SceneType::Test,   []() { return std::make_unique<Test>(); });
	sceneManager_->RegisterScene(SceneType::Title,  []() { return std::make_unique<Title>(); });
	sceneManager_->RegisterScene(SceneType::Game,   []() { return std::make_unique<Game>(); });
	sceneManager_->RegisterScene(SceneType::Result, []() { return std::make_unique<Result>(); });
	sceneManager_->Initialize(SceneType::Test);
}

void Terminal::Run() {

	while (execution_->IsRunning()) {

		execution_->Update();

		sceneManager_->Update();

		execution_->PreDraw();

		sceneManager_->Draw();

		// DockSpaceを先に生成してから、シーンのImGuiウィンドウを作成する
		execution_->BeginImGuiLayout();

		sceneManager_->DrawImGui();

		execution_->PostDraw();
	}

}