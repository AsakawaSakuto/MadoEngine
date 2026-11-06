#pragma once
#include "SceneManager/SceneType.h"
#include <filesystem>
#include <string>

namespace MadoEngine::Editor {

inline const std::filesystem::path kModelEditorJsonPath = "Assets/Json/ModelObjects.json";
inline const std::filesystem::path kSpriteEditorJsonPath = "Assets/Json/SpriteObjects.json";

/// @brief ModelEditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadModelEditorJson();

/// @brief 指定シーン所属のModel Editor設定をJsonから読み込み
/// @param sceneType 読み込み対象のシーン、SceneType::None所属のModelも読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadModelEditorJson(SceneType sceneType);

/// @brief Sprite EditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadSpriteEditorJson();

/// @brief 指定シーン所属のSprite Editor設定をJsonから読み込み
/// @param sceneType 読み込み対象のシーン、SceneType::None所属のSpriteも読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadSpriteEditorJson(SceneType sceneType);

/// @brief TextEditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadTextEditorJson();

/// @brief 指定シーン所属のText Editor設定をJsonから読み込み
/// @param sceneType 読み込み対象のシーン、SceneType::None所属のTextも読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadTextEditorJson(SceneType sceneType);

/// @brief LightEditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadLightEditorJson();

/// @brief 同じシーンの描画要素を復元した後にGroup設定を読込
/// @param sceneType 読込対象のシーン
/// @return 読込に成功した場合はtrue
bool LoadUIGroupEditorJson(SceneType sceneType);
/// @brief Groupと参照する描画要素の編集用設定を保存
/// @param sceneType 保存対象のシーン
/// @return 全ての保存に成功した場合はtrue
bool SaveUIGroupEditorJson(SceneType sceneType);

/// @brief 新規要素の生成情報を含むGroup Editorの履歴を取得
/// @return Group・Sprite・Textの編集用Snapshot文字列
std::string CaptureUIGroupEditorState();
/// @brief 新規作成と削除も含めてGroup Editorの履歴を復元
/// @param snapshot 復元するSnapshot文字列
/// @return 形式を検証して復元できた場合はtrue
bool RestoreUIGroupEditorState(const std::string& snapshot);

} // namespace MadoEngine::Editor
