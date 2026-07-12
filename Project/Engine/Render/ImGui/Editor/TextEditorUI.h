#pragma once
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

/// @brief TextEditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadTextEditorJson();

#ifdef USE_IMGUI

/// @brief TextManager用のEditor UIを描画します。
void DrawTextManagerEditorUI();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
