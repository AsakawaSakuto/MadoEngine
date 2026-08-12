#pragma once

#include ".SceneManager/SceneType.h"
#include "Utility/Camera/CameraManager.h"

#ifdef USE_IMGUI

namespace MadoEngine::Editor {

/// @brief SceneローカルCameraManagerのEditor UIを描画
/// @param cameraManager 編集対象のCameraManager
/// @param currentSceneType 現在のScene種別
void DrawCameraManagerEditorUI(CameraManager& cameraManager, SceneType currentSceneType);

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
