#pragma once

#ifdef USE_IMGUI

#include "../../../ImGuiHeaders.h"
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
#include "../../../RenderHeaders.h"
#include <string>
namespace MadoEngine::Editor {

/// @brief PostEffect EditorのSnapshot復元を次Frame描画前へ予約
/// @param snapshot 復元するSnapshot文字列
void ReservePostEffectEditorStateRestore(const std::string& snapshot);

/// @brief 予約されたPostEffect Editor操作を適用
/// @param postEffectManager 適用対象のポストエフェクト管理クラス
void ApplyPendingPostEffectEditorOperations(Render::PostEffectManager& postEffectManager);

/// @brief PostEffect Editorを描画
/// @param postEffectManager 編集対象のポストエフェクト管理クラス
void DrawPostEffectEditorUI(Render::PostEffectManager& postEffectManager);

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
