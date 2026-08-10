#pragma once
#include ".SceneManager/SceneType.h"
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

/// @brief TextEditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadTextEditorJson();

/// @brief 指定シーン所属のText Editor設定をJsonから読み込み
/// @param sceneType 読み込み対象のシーン、SceneType::None所属のTextも読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadTextEditorJson(SceneType sceneType);

#ifdef USE_IMGUI

/// @brief TextManager用のEditor UIを描画
/// @param currentSceneType 現在のシーン
void DrawTextManagerEditorUI(SceneType currentSceneType);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
