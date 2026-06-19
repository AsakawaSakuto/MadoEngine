#pragma once
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif

namespace MadoEngine::Editor {

#ifdef USE_IMGUI

	/// @brief AudioManagerの内容を表示・操作するImGui UIを描画する
	void DrawAudioManagerUI();

	/// @brief Layer Effect Pass Editorを描画する
	/// @param postEffectManager 編集対象のポストエフェクト管理クラス
	void DrawLayerEffectPassEditorUI(Render::PostEffectManager& postEffectManager);

	/// @brief LightManager Editorを描画する
	void DrawLightManagerEditorUI();

	/// @brief Game View上にTransformギズモを描画し、Transformへ反映する
	/// @param camera ギズモ表示に使用するカメラ
	/// @param transform 操作対象のTransform
	/// @return ギズモ操作でTransformが変更された場合はtrue
	bool DrawTransformGizmoOnGameView(const Camera& camera, Transform3D& transform);

	/// @brief Game View上でModelをクリック選択し、選択中ModelのTransformギズモを描画する
	/// @param camera 選択レイとギズモ表示に使用するカメラ
	/// @param sceneType 選択対象のシーン種別
	/// @param selectedModel 選択中Modelのポインタ
	/// @return 選択またはTransformが変更された場合はtrue
	bool DrawModelGizmoOnGameView(const Camera& camera, SceneType sceneType, Model*& selectedModel);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
