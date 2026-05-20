#include "Terminal.h"

Terminal::Terminal(HINSTANCE hInstance)
{
	execution_ = std::make_unique<MadoEngine::Execution>();
	execution_->Initialize(hInstance);
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->RegisterScene("Test",   []() { return std::make_unique<Test>(); });
	sceneManager_->RegisterScene("Title",  []() { return std::make_unique<Title>(); });
	sceneManager_->RegisterScene("Game",   []() { return std::make_unique<Game>(); });
	sceneManager_->RegisterScene("Result", []() { return std::make_unique<Result>(); });
	sceneManager_->Initialize("Test");
}

void Terminal::Run() {

	while (execution_->IsRunning()) {

		execution_->Update();

		sceneManager_->Update();

		execution_->PreDraw();

		sceneManager_->Draw();

		execution_->PostDraw();
	}

}