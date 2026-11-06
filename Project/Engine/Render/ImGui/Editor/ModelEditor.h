#pragma once

#ifdef USE_IMGUI

#include "SceneManager/SceneType.h"

#include "../../../ImGuiHeaders.h"

namespace MadoEngine::Editor {

/// @brief ModelManager用のEditor UIを描画
/// @param currentSceneType 現在のシーン
void DrawModelManagerEditorUI(SceneType currentSceneType);

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
