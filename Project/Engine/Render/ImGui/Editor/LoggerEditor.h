#pragma once
#include "../../../ImGuiHeaders.h"
#include "../../../Utility/Logger/Logger.h"

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

/// @brief Loggerの履歴を表示するImGui UIを描画する
void DrawLoggerEditorUI();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
