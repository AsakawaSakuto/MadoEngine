#pragma once
#include <string>

namespace MadoEngine::Editor {

/// @brief AudioEditorの現在状態をメモリ内Snapshotへ変換
/// @return AudioEditorのSnapshot文字列
std::string CaptureAudioEditorState();

/// @brief メモリ内SnapshotからAudioEditorの状態を復元
/// @param snapshot 復元元のSnapshot文字列
/// @return 復元に成功した場合はtrue
bool RestoreAudioEditorState(const std::string& snapshot);

/// @brief AudioEditorのJson設定を保存
/// @return 保存に成功した場合はtrue
bool SaveAudioEditorJson();

/// @brief AudioEditorのJson設定を読み込み
/// @return 読み込みに成功した場合はtrue
bool LoadAudioEditorJson();

} // namespace MadoEngine::Editor
