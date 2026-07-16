#pragma once

#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

/// @brief ModelEditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadModelEditorJson();

#ifdef USE_IMGUI

/// @brief ModelManager用のEditor UIを描画する
void DrawModelManagerEditorUI();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
