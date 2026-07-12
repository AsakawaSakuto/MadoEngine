#pragma once
#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
#include "../../../RenderHeaders.h"
namespace MadoEngine::Editor {

/// @brief Layer Effect Pass EditorのJson設定を読み込む
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadLayerEffectPassEditorJsonFromFile(Render::PostEffectManager& postEffectManager);

/// @brief Layer Effect Pass EditorのJson設定を読み込む
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadLayerEffectPassEditorJson(Render::PostEffectManager& postEffectManager);

#ifdef USE_IMGUI

/// @brief Layer Effect Pass Editorを描画する
/// @param postEffectManager 編集対象のポストエフェクト管理クラス
void DrawLayerEffectPassEditorUI(Render::PostEffectManager& postEffectManager);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
