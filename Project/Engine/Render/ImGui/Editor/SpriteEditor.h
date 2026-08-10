#pragma once

#include ".SceneManager/SceneType.h"

#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

/// @brief Sprite EditorのJson設定を読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadSpriteEditorJson();

/// @brief 指定シーン所属のSprite Editor設定をJsonから読み込む
/// @param sceneType 読み込み対象のシーン。SceneType::None所属のSpriteも読み込む
/// @return 読み込みに成功した場合はtrue
bool LoadSpriteEditorJson(SceneType sceneType);

#ifdef USE_IMGUI

/// @brief SpriteManager用のEditor UIを描画する
/// @param currentSceneType 現在のシーン
void DrawSpriteManagerEditorUI(SceneType currentSceneType);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
