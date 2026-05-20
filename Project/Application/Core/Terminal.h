#pragma once
#include ".Execution/Execution.h"
#include "SceneManager/SceneManager.h"
#include "SceneManager/Scene/Test.h"
#include "SceneManager/Scene/Title.h"
#include "SceneManager/Scene/Game.h"
#include "SceneManager/Scene/Result.h"

class Terminal {
public:

	Terminal(HINSTANCE hInstance);

	void Run();

private:
	std::unique_ptr<MadoEngine::Execution> execution_;
	std::unique_ptr<SceneManager> sceneManager_;
};