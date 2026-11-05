#include "Transform2DEditor.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"

namespace MadoEngine::Editor {

bool DrawTransform2DInspector(TransformNode2D& node, bool showOpacity) {
	auto state = node.GetAuthoringState();
	bool changed = false;
	float position[2] = { state.transform.translate.x, state.transform.translate.y };
	float scale[2] = { state.transform.scale.x, state.transform.scale.y };
	float degrees = state.transform.rotate * 57.2957795131f;
	changed |= ImGui::DragFloat2("位置", position, 1.0f);
	changed |= ImGui::DragFloat2("スケール", scale, 0.01f);
	changed |= ImGui::DragFloat("回転（度）", &degrees, 0.5f);
	if (showOpacity) {
		changed |= ImGui::SliderFloat("不透明度", &state.opacity, 0.0f, 1.0f);
	}

	// 通常Setterと編集用設定を分離し、再生中のアニメーションを保存対象から除外
	if (changed) {
		state.transform.translate = { position[0], position[1] };
		state.transform.scale = { scale[0], scale[1] };
		state.transform.rotate = degrees * 0.01745329252f;
		if (node.HasAuthoringState()) {
			node.SetAuthoringState(state);
		} else {
			node.SetTransform(state.transform);
			node.SetVisible(state.visible);
			node.SetOpacity(state.opacity);
		}
	}
	return changed;
}

void SetEditorNodeVisible(TransformNode2D& node, bool visible) {

	// 親の一時非表示と個別の保存用表示状態を独立して編集
	if (node.HasAuthoringState()) {
		auto state = node.GetAuthoringState();
		state.visible = visible;
		node.SetAuthoringState(state);
	} else {
		node.SetVisible(visible);
	}
}

} // namespace MadoEngine::Editor
#endif
