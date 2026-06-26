#pragma once
#include "../../../ImGuiHeaders.h"
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
#include "../../../RenderHeaders.h"
namespace MadoEngine::Editor {

#ifdef USE_IMGUI

/// @brief Layer Effect Pass Editorを描画する
/// @param postEffectManager 編集対象のポストエフェクト管理クラス
void DrawLayerEffectPassEditorUI(Render::PostEffectManager& postEffectManager);

/// @brief Layer Effect Pass EditorのJson設定を読み込む
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadLayerEffectPassEditorJsonFromFile(Render::PostEffectManager& postEffectManager);

/// @brief Layer Effect Pass EditorのJson設定を読み込む
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadLayerEffectPassEditorJson(Render::PostEffectManager& postEffectManager);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
