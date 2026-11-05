#pragma once

#include "SceneManager/SceneType.h"
#include "Render/Object/ObjectHandle.h"
#include <string>

class Sprite;

#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

/// @brief Sprite EditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadSpriteEditorJson();

/// @brief 指定シーン所属のSprite Editor設定をJsonから読み込み
/// @param sceneType 読み込み対象のシーン、SceneType::None所属のSpriteも読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadSpriteEditorJson(SceneType sceneType);

#ifdef USE_IMGUI

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

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
