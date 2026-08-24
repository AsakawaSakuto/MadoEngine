#pragma once
#ifdef USE_IMGUI
#include "../../../ImGuiHeaders.h"
#endif
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
#include "../../../RenderHeaders.h"
#include <string>
namespace MadoEngine::Editor {

/// @brief PostEffect Editorの現在状態をメモリ内Snapshotへ変換
/// @param postEffectManager 取得対象のPostEffectManager
/// @return PostEffect EditorのSnapshot文字列
std::string CapturePostEffectEditorState(const Render::PostEffectManager& postEffectManager);

/// @brief メモリ内SnapshotからPostEffect Editorの状態を復元
/// @param postEffectManager 復元対象のPostEffectManager
/// @param snapshot 復元元のSnapshot文字列
/// @return 復元に成功した場合はtrue
bool RestorePostEffectEditorState(
	Render::PostEffectManager& postEffectManager,
	const std::string& snapshot
);

/// @brief PostEffect EditorのJson設定を保存
/// @param postEffectManager 保存対象のPostEffectManager
/// @return 保存に成功した場合はtrue
bool SavePostEffectEditorJsonToFile(const Render::PostEffectManager& postEffectManager);

/// @brief PostEffect EditorのJson設定を読み込み
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadPostEffectEditorJsonFromFile(Render::PostEffectManager& postEffectManager);

/// @brief PostEffect EditorのJson設定を読み込み
/// @param postEffectManager 読み込み先のPostEffectManager
/// @return 読み込みに成功した場合はtrue
bool LoadPostEffectEditorJson(Render::PostEffectManager& postEffectManager);

#ifdef USE_IMGUI

/// @brief PostEffect EditorのSnapshot復元を次Frame描画前へ予約
/// @param snapshot 復元するSnapshot文字列
void ReservePostEffectEditorStateRestore(const std::string& snapshot);

/// @brief 予約されたPostEffect Editor操作を適用
/// @param postEffectManager 適用対象のポストエフェクト管理クラス
void ApplyPendingPostEffectEditorOperations(Render::PostEffectManager& postEffectManager);

/// @brief PostEffect Editorを描画
/// @param postEffectManager 編集対象のポストエフェクト管理クラス
void DrawPostEffectEditorUI(Render::PostEffectManager& postEffectManager);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
