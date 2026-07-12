#pragma once
#include "../../../Audio/AudioManager.h"
#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif
namespace MadoEngine::Editor {

/// @brief AudioEditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadAudioEditorJson();

#ifdef USE_IMGUI

/// @brief AudioManagerの内容を表示・操作するImGui UIを描画する
void DrawAudioManagerUI();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
