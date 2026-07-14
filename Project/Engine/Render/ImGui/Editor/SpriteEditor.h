#pragma once

#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

/// @brief Sprite EditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadSpriteEditorJson();

#ifdef USE_IMGUI

/// @brief SpriteManager用のEditor UIを描画する
void DrawSpriteManagerEditorUI();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
