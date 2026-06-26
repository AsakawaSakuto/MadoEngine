#pragma once
#include "../../../Audio/AudioManager.h"
#include "../../../ImGuiHeaders.h"
namespace MadoEngine::Editor {

#ifdef USE_IMGUI

/// @brief AudioManagerの内容を表示・操作するImGui UIを描画する
void DrawAudioManagerUI();

/// @brief AudioEditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadAudioEditorJson();

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
