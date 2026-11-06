#pragma once
#include "Render/PostEffectManager.h"
#include "Render/PostEffect/PostEffectDefinitionRegistry.h"
#include <filesystem>
#include <string>

namespace MadoEngine::Editor {

// 既存設定との互換性を維持する保存先
inline const std::filesystem::path kPostEffectEditorJsonPath = "Assets/Json/LayerEffectPassEditor.json";

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

/// @brief 指定ファイルからポストエフェクト設定を読み込み
/// @param postEffectManager 読み込み先のポストエフェクト管理
/// @param filePath 読み込む設定ファイルのパス
/// @return 読み込みに成功した場合はtrue
bool LoadPostEffectEditorJsonFromFile(
	Render::PostEffectManager& postEffectManager,
	const std::filesystem::path& filePath);

namespace Detail {

/// @brief レイヤーポストエフェクトの適用段階名を取得
/// @param stage 名前を取得する適用段階
/// @return 適用段階名
const char* GetLayerEffectStageName(Render::LayerEffectStage stage);

/// @brief フルスクリーンポストエフェクトの適用段階名を取得
/// @param stage 名前を取得する適用段階
/// @return 適用段階名
const char* GetScreenEffectStageName(Render::ScreenEffectStage stage);

/// @brief Passに設定された登録済みポストエフェクト定義を取得
/// @param pass 検索対象のPass
/// @return 登録済み定義、未登録Shaderの場合はnullptr
const Render::PostEffectDefinition* FindPostEffectDefinition(const Render::PostEffectPass& pass);

/// @brief 指定したJSONパスのバックアップパスを作成
/// @param filePath バックアップ元のJSONパス
/// @return .bakを付けたバックアップJSONパス
std::filesystem::path CreateBackupJsonPath(const std::filesystem::path& filePath);

} // namespace Detail

} // namespace MadoEngine::Editor
