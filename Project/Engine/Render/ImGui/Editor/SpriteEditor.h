#pragma once

#ifdef USE_IMGUI

#include "SceneManager/SceneType.h"
#include "Render/Object/ObjectHandle.h"
#include <string>

class Sprite;

#include "../../../ImGuiHeaders.h"

namespace MadoEngine::Editor {

/// @brief 次のSprite Editor描画で選択対象を切り替え
/// @param handle 選択するSpriteのハンドル
void SelectSpriteEditorObject(SpriteHandle handle);

/// @brief 別のEditor内でSpriteの詳細Inspectorを描画
/// @param sprite 編集対象のSprite
void DrawSpriteInspector(::Sprite& sprite);

/// @brief 共通のテクスチャ選択UIを描画
/// @param label 選択UIのラベル
/// @param textureName 選択中のテクスチャ名
/// @return 選択を変更した場合はtrue
bool DrawSpriteTextureSelector(const char* label, std::string& textureName);

/// @brief SpriteManager用のEditor UIを描画
/// @param currentSceneType 現在のシーン
void DrawSpriteManagerEditorUI(SceneType currentSceneType);

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
