#pragma once

#ifdef USE_IMGUI

#include "SceneManager/SceneType.h"
#include <string>

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

#endif // USE_IMGUI
