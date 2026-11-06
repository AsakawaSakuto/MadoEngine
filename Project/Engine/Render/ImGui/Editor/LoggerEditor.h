#pragma once

#ifdef USE_IMGUI

#include "../../../ImGuiHeaders.h"
#include "../../../Utility/Logger/Logger.h"

namespace MadoEngine::Editor {

/// @brief Loggerの履歴を表示するImGui UIを描画
void DrawLoggerEditorUI();

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
