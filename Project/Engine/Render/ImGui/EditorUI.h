#pragma once
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif
namespace MadoEngine::Editor {

#ifdef USE_IMGUI

	/// @brief AudioManagerの内容を表示・操作するImGui UIを描画する関数
    void DrawAudioManagerUI();

#endif // USE_IMGUI
}