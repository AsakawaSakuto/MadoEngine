#include "Terminal.h"

Terminal::Terminal(HINSTANCE hInstance)
{
	execution_ = std::make_unique<MadoEngine::Execution>();
	execution_->Initialize(hInstance);
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->Initialize();
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