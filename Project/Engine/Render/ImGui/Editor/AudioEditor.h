#pragma once

#ifdef USE_IMGUI

#include <string>
#include "../../../ImGuiHeaders.h"
namespace MadoEngine::Editor {

/// @brief AudioManagerの内容を表示・操作するImGui UIを描画
void DrawAudioManagerUI();

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
