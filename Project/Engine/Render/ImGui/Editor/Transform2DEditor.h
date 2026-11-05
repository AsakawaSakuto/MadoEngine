#pragma once

#ifdef USE_IMGUI
#include "Math/TransformNode2D.h"

namespace MadoEngine::Editor {

/// @brief 編集用設定を優先した2Dローカル設定の共通Inspectorを描画
/// @param node 編集するノード
/// @param showOpacity 不透明度の編集欄を表示する場合はtrue
/// @return 設定を変更した場合はtrue
bool DrawTransform2DInspector(TransformNode2D& node, bool showOpacity = true);

/// @brief 個別の表示設定を編集用設定にも反映
/// @param node 編集するノード
/// @param visible 表示する場合はtrue
void SetEditorNodeVisible(TransformNode2D& node, bool visible);

} // namespace MadoEngine::Editor
#endif
