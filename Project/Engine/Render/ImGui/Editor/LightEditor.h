#pragma once
#include "../../../ImGuiHeaders.h"
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
namespace MadoEngine::Editor {

#ifdef USE_IMGUI

/// @brief LightManager Editorを描画する
void DrawLightManagerEditorUI();

/// @brief LightEditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadLightEditorJson();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
