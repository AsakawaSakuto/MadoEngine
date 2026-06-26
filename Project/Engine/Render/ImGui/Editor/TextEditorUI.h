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

/// @brief TextEditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadTextEditorJson();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
