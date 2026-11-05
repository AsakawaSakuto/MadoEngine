#pragma once
#include "SceneManager/SceneType.h"
#include <string>

namespace MadoEngine::Editor {

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

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#include "Render/Object/ObjectHandle.h"
namespace MadoEngine::Editor {

/// @brief 外部のEditorからGroupまたは直接の子を選択
/// @param handle 選択するGroup
/// @param elementName 選択する子の登録名、空文字列の場合はGroup自身
void SelectUIGroupEditorObject(UIGroupHandle handle, const std::string& elementName = {});

/// @brief 階層・所属・編集用ローカル設定のEditorを描画
/// @param currentSceneType 現在のシーン
void DrawUIGroupEditorUI(SceneType currentSceneType);
/// @brief Game View画像内へ選択ノードの原点と軸を描画
/// @param imageMin 画像左上のImGui座標
/// @param imageSize 画像のImGui表示サイズ
void DrawUIGroupOriginOverlay(const ImVec2& imageMin, const ImVec2& imageSize);

} // namespace MadoEngine::Editor
#endif
