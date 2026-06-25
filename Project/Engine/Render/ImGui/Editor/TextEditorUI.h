#pragma once
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

/// @brief TextManager用のEditor UIを描画します。
void DrawTextManagerEditorUI();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
