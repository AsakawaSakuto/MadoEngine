#pragma once
#include "MathHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#include "ImGuiHeaders.h"
#include "SceneType.h"

class IScene
{
public:
	virtual ~IScene() = default;

	virtual void Initialize() = 0;
	virtual SceneType Update(float dt) = 0;
	virtual void Draw() = 0;
	virtual void DrawImGui() = 0;
	virtual void Finalize() = 0;
};
