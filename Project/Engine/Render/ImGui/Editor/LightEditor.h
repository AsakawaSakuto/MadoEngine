#pragma once

#ifdef USE_IMGUI

#include "../../../ImGuiHeaders.h"
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
namespace MadoEngine::Editor {

/// @brief LightManager Editorを描画
/// @param currentSceneType 現在のシーン
void DrawLightManagerEditorUI(SceneType currentSceneType);

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
