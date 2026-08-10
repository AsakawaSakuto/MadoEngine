#pragma once

#include ".SceneManager/SceneType.h"

#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

/// @brief ModelEditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadModelEditorJson();

/// @brief 指定シーン所属のModel Editor設定をJsonから読み込み
/// @param sceneType 読み込み対象のシーン、SceneType::None所属のModelも読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadModelEditorJson(SceneType sceneType);

#ifdef USE_IMGUI

/// @brief ModelManager用のEditor UIを描画
/// @param currentSceneType 現在のシーン
void DrawModelManagerEditorUI(SceneType currentSceneType);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
