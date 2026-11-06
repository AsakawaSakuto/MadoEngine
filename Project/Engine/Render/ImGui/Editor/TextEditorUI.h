#pragma once

#ifdef USE_IMGUI

#include "SceneManager/SceneType.h"
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#include "ImGuiHeaders.h"

namespace MadoEngine::Editor {

/// @brief 次のText Editor描画で選択対象を切り替え
/// @param handle 選択するTextのハンドル
void SelectTextEditorObject(TextHandle handle);

/// @brief 別のEditor内でTextの本文・フォント・描画設定を編集
/// @param text 編集対象のText
void DrawTextInspector(Text& text);

/// @brief TextManager用のEditor UIを描画
/// @param currentSceneType 現在のシーン
void DrawTextManagerEditorUI(SceneType currentSceneType);

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
