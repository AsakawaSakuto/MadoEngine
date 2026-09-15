#pragma once
#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
namespace MadoEngine::Editor {

/// @brief LightEditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadLightEditorJson();

#ifdef USE_IMGUI

/// @brief LightManager Editorを描画
/// @param currentSceneType 現在のシーン
void DrawLightManagerEditorUI(SceneType currentSceneType);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
