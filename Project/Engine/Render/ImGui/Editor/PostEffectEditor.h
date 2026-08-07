#pragma once
#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
#include "../../../RenderHeaders.h"
namespace MadoEngine::Editor {

/// @brief PostEffect EditorのJson設定を読み込む
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadPostEffectEditorJsonFromFile(Render::PostEffectManager& postEffectManager);

/// @brief PostEffect EditorのJson設定を読み込む
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadPostEffectEditorJson(Render::PostEffectManager& postEffectManager);

#ifdef USE_IMGUI

/// @brief 予約されたPostEffect Editor操作を適用する
/// @param postEffectManager 適用対象のポストエフェクト管理クラス
void ApplyPendingPostEffectEditorOperations(Render::PostEffectManager& postEffectManager);

/// @brief PostEffect Editorを描画する
/// @param postEffectManager 編集対象のポストエフェクト管理クラス
void DrawPostEffectEditorUI(Render::PostEffectManager& postEffectManager);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
