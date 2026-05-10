#pragma once
#include ".Execution/Execution.h"
#include "SceneManager/SceneManager.h"

class Terminal {
public:

	Terminal(HINSTANCE hInstance);

	void Run();

private:
	std::unique_ptr<MadoEngine::Execution> execution_;
	std::unique_ptr<SceneManager> sceneManager_;
};