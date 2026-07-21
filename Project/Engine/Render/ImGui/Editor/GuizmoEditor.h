#pragma once
#include "../../../ImGuiHeaders.h"
#include "../../../UtilityHeaders.h"
#include "../../../MathHeaders.h"
#include "../../../RenderHeaders.h"
namespace MadoEngine::Editor {

#ifdef USE_IMGUI

/// @brief Game View上にTransformギズモを描画し、Transformへ反映する
/// @param camera ギズモ表示に使用するカメラ
/// @param transform 操作対象のTransform
/// @return ギズモ操作でTransformが変更された場合はtrue
bool DrawTransformGizmoOnGameView(const Camera& camera, Transform3D& transform);

/// @brief Scene遷移前にModelギズモが保持しているModel参照と編集履歴を破棄する
/// @param selectedModel 現在選択中のModelポインタ
void ResetModelGizmoOnSceneChange(Model*& selectedModel);

/// @brief Game View上でModelをクリック選択し、選択中ModelのTransformギズモを描画する
/// @param camera 選択レイとギズモ表示に使用するカメラ
/// @param sceneType 選択対象のシーン種別
/// @param selectedModel 選択中Modelのポインタ
/// @return 選択またはTransformが変更された場合はtrue
bool DrawModelGizmoOnGameView(const Camera& camera, SceneType sceneType, Model*& selectedModel);

#endif // USE_IMGUI

} // namespace MadoEngine::Editor