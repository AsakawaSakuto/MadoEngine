#include "UIGroupEditor.h"

#ifdef USE_IMGUI

#include "Utility/Settings/SceneObjectSettings.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Render/Object/2d/Text/TextManager.h"

#include "EditorToolbar.h"
#include "SpriteEditor.h"
#include "TextEditorUI.h"
#include "Transform2DEditor.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <unordered_set>

namespace MadoEngine::Editor {
namespace {

constexpr float kHierarchyMinWidth = 180.0f;
constexpr float kInspectorMinWidth = 220.0f;
constexpr float kHierarchySplitterWidth = 8.0f;

struct EditorSession {
	UIGroupHandle selected;
	UIGroupHandle editing;
	UIGroupHandle deleteOwner;
	std::string alias;
	std::string deleteAlias;
	std::string editingAlias;
	std::string displayedGroupName;
	std::array<char, 128> createName{ "UIGroup" };
	std::array<char, 128> groupName{};
	std::array<char, 128> aliasName{};
	std::array<char, 128> newAlias{ "Element" };
	std::array<char, 128> createElementName{ "Sprite" };
	int createType = 0;
	std::string target;
	int type = 0;
	float hierarchyWidth = 240.0f;
	bool global = false;
	bool showOrigin = true;
};

EditorSession session;

/// @brief 選択変更時に文字列の編集バッファを同期
/// @param buffer 同期するバッファ
/// @param value コピーする文字列
void CopyBuffer(std::array<char, 128>& buffer, const std::string& value) {
	strncpy_s(buffer.data(), buffer.size(), value.c_str(), _TRUNCATE);
}

/// @brief Groupの表示対象シーンを判定
/// @param handle 対象のGroup
/// @param scene 現在のシーン
/// @return 現在シーンまたは共通Groupの場合はtrue
bool IsInScene(UIGroupHandle handle, SceneType scene) {
	const auto owner = UIGroupManager::GetInstance().GetSceneType(handle);
	return owner == SceneType::None || owner == scene;
}

/// @brief 接続先の種類を表示用文字列へ変換
/// @param type 要素の種類
/// @return 種類の表示名
const char* ElementLabel(UIGroupElementType type) {
	if (type == UIGroupElementType::Sprite) {
		return "Sprite";
	}
	if (type == UIGroupElementType::Text) {
		return "Text";
	}
	return "Group";
}

/// @brief 名前の左側に配置する表示チェックボックスを描画
/// @param node 表示設定の編集対象、未接続の場合はnullptr
void DrawNodeVisibilityCheckbox(TransformNode2D* node) {
	bool visible = false;
	if (node) {
		visible = node->GetAuthoringState().visible;
	}

	// 親の一時非表示と子の個別設定を混同せず、未接続の要素は編集を無効化
	ImGui::BeginDisabled(!node);
	if (ImGui::Checkbox("##Visible", &visible) && node) {
		SetEditorNodeVisible(*node, visible);
	}
	ImGui::EndDisabled();
}

/// @brief 子要素の削除予約ボタンを描画
/// @param owner 削除する要素の親Group
/// @param name 削除する要素の登録名
void DrawElementDeleteButton(UIGroupHandle owner, const std::string& name) {
	if (ImGui::SmallButton("削除")) {

		// 階層の走査中にGroupを破棄しないよう、削除をツリー描画後まで保留
		session.deleteOwner = owner;
		session.deleteAlias = name;
	}
}

/// @brief 選択中ノードの階層ツリーと子要素の削除ボタンを描画
/// @param handle 表示するGroup
/// @param label ツリー上の表示名
/// @param visited 表示済みGroupの添字
/// @param depth 現在の階層の深さ
/// @param owner 子Groupの削除元、ルートGroupの場合は無効ハンドル
/// @param elementName 親Group内の登録名、ルートGroupの場合は空文字列
void DrawGroupTree(UIGroupHandle handle, const std::string& label, std::unordered_set<uint32_t>& visited, int depth,
	UIGroupHandle owner = {}, const std::string& elementName = {}) {
	auto& manager = UIGroupManager::GetInstance();

	// 外部から変更された構成でも重複表示や過大な再帰を回避
	if (!manager.TryGet(handle) || !visited.insert(handle.index).second || depth > 64) {
		return;
	}
	ImGui::PushID(static_cast<int>(handle.index));
	DrawNodeVisibilityCheckbox(manager.TryGet(handle));
	ImGui::SameLine();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	const float deleteButtonWidth = ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
	const float deleteButtonX = ImGui::GetCursorPosX() + (std::max)(0.0f, ImGui::GetContentRegionAvail().x - deleteButtonWidth);
	if (owner.IsValid()) {

		// 子Groupの行全体の選択領域と右端の削除ボタンの重なりを許可
		flags |= ImGuiTreeNodeFlags_AllowOverlap;
	}
	if (handle == session.selected && session.alias.empty()) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	const bool open = ImGui::TreeNodeEx("Group", flags, "%s", label.c_str());
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		session.selected = handle;
		session.alias.clear();
	}
	if (owner.IsValid()) {
		ImGui::SameLine(deleteButtonX);
		DrawElementDeleteButton(owner, elementName);
	}
	if (open) {
		for (const auto& element : manager.GetElements(handle)) {
			if (element.type == UIGroupElementType::Group && element.resolved) {
				DrawGroupTree(element.group, element.name + " [" + manager.GetName(element.group) + "]", visited, depth + 1,
					handle, element.name);
			} else {
				ImGui::PushID(element.name.c_str());
				DrawNodeVisibilityCheckbox(manager.TryGetElement(handle, element.name));
				ImGui::SameLine();
				std::string row = element.name + " [" + ElementLabel(element.type) + "]";
				if (!element.resolved) {
					row += "（未接続）";
				}

				// 選択領域の幅からボタン分を除き、削除操作による行選択との干渉を防止
				const float selectableWidth = (std::max)(1.0f,
					ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x);
				if (ImGui::Selectable(row.c_str(), session.selected == handle && session.alias == element.name, 0, { selectableWidth, 0.0f })) {
					session.selected = handle;
					session.alias = element.name;
				}
				ImGui::SameLine();
				DrawElementDeleteButton(handle, element.name);
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

/// @brief 新規作成する要素の未使用名を生成
/// @param handle 追加先のGroup
/// @param baseName 要素名の基底文字列
/// @return Group内で未使用の要素名
std::string NextElementName(UIGroupHandle handle, const std::string& baseName) {
	const auto elements = UIGroupManager::GetInstance().GetElements(handle);
	std::string name = baseName;
	uint32_t suffix = 1;

	// 未接続の参照にも同じ名前を割り当てないための一意性確認
	while (std::any_of(elements.begin(), elements.end(), [&](const auto& element) { return element.name == name; })) {
		name = baseName + std::to_string(suffix++);
	}
	return name;
}

/// @brief Group内でSprite・Text・子Groupを新規作成
/// @param handle 作成先のGroup
void DrawCreation(UIGroupHandle handle) {
	auto& manager = UIGroupManager::GetInstance();
	ImGui::PushID("CreateElement");
	ImGui::SeparatorText("新規要素の追加");
	if (ImGui::Combo("種類", &session.createType, "Sprite\0Text\0Group\0")) {
		CopyBuffer(session.createElementName, NextElementName(handle, ElementLabel(static_cast<UIGroupElementType>(session.createType))));
	}
	ImGui::InputText("要素名", session.createElementName.data(), session.createElementName.size());
	const bool canCreate = session.createElementName[0] != '\0';
	ImGui::BeginDisabled(!canCreate);
	if (ImGui::Button("作成して追加")) {
		const std::string name = session.createElementName.data();
		bool success = false;

		// 作成時は既定値を使用し、詳細設定を追加後のInspectorへ集約
		if (session.createType == static_cast<int>(UIGroupElementType::Sprite)) {
			success = manager.CreateSpriteElement(handle, name, "white2x2").IsValid();
		} else if (session.createType == static_cast<int>(UIGroupElementType::Text)) {
			success = manager.CreateTextElement(handle, name, "Text").IsValid();
		} else {
			success = manager.CreateGroupElement(handle, name).IsValid();
		}
		EditorToolbar::GetInstance().NotifyOperationResult("要素の新規作成", success);
		if (success) {
			session.alias = name;
			session.editing = {};
			CopyBuffer(session.createElementName, NextElementName(handle, ElementLabel(static_cast<UIGroupElementType>(session.createType))));
		}
	}
	ImGui::EndDisabled();
	ImGui::PopID();
}

/// @brief 既存要素を再利用する補助的な登録UIを描画
/// @param handle 登録先のGroup
void DrawRegistration(UIGroupHandle handle) {
	auto& manager = UIGroupManager::GetInstance();
	const auto scene = manager.GetSceneType(handle);
	ImGui::SeparatorText("子要素の登録");
	ImGui::TextDisabled("同じシーンのEditor管理要素を登録（現在値をローカル値として使用）");
	if (ImGui::Combo("種類", &session.type, "Sprite\0Text\0Group\0")) {
		session.target.clear();
	}
	std::vector<std::pair<std::string, std::string>> candidates;
	const auto type = static_cast<UIGroupElementType>(session.type);

	// RuntimeOnlyの要素は保存対象にせず、ゲーム固有の既存UIを巻き込まないための絞り込み
	if (type == UIGroupElementType::Sprite) {
		for (const auto& name : SpriteManager::GetInstance().GetEditorManagedNames()) {
			auto* node = SpriteManager::GetInstance().TryGet(SpriteManager::GetInstance().Find(name));
			if (node && node->GetSceneType() == scene && !node->HasParent()) {
				candidates.emplace_back(name, name);
			}
		}
	} else if (type == UIGroupElementType::Text) {
		for (const auto& name : TextManager::GetInstance().GetEditorManagedNames()) {
			auto* node = TextManager::GetInstance().TryGet(TextManager::GetInstance().Find(name));
			if (node && node->GetSceneType() == scene && !node->HasParent()) {
				candidates.emplace_back(name, name);
			}
		}
	} else {
		for (auto candidate : manager.GetHandles()) {
			if (candidate != handle && manager.GetSceneType(candidate) == scene && !manager.TryGet(candidate)->HasParent()) {
				candidates.emplace_back(manager.GetId(candidate), manager.GetName(candidate));
			}
		}
	}
	std::string preview = "選択してください";
	for (const auto& [target, label] : candidates) {
		if (target == session.target) {
			preview = label;
		}
	}
	if (ImGui::BeginCombo("登録対象", preview.c_str())) {
		for (const auto& [target, label] : candidates) {
			if (ImGui::Selectable(label.c_str(), target == session.target)) {
				session.target = target;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::InputText("登録名", session.newAlias.data(), session.newAlias.size());
	if (ImGui::Button("登録")) {
		const bool success = manager.AddElement(handle, session.newAlias.data(), type, session.target);
		EditorToolbar::GetInstance().NotifyOperationResult("要素の登録（名前・所属・循環を確認）", success);
	}
}

/// @brief 子要素の設定編集と削除を描画
/// @param handle 現在の親Group
void DrawElementInspector(UIGroupHandle handle) {
	auto& manager = UIGroupManager::GetInstance();
	if (ImGui::Button("Group設定へ戻る")) {
		session.alias.clear();
		return;
	}
	const auto elements = manager.GetElements(handle);
	const auto it = std::find_if(elements.begin(), elements.end(), [](const auto& element) { return element.name == session.alias; });
	if (it == elements.end()) {
		session.alias.clear();
		return;
	}
	std::string targetLabel = it->target;
	if (it->type != UIGroupElementType::Group) {
		targetLabel = session.alias;
	}
	if (it->type == UIGroupElementType::Group) {
		targetLabel = manager.GetName(it->group);
		if (targetLabel.empty()) {
			targetLabel = "未接続のGroup";
		}
	}
	ImGui::Text("%s: %s", ElementLabel(it->type), targetLabel.c_str());
	const bool submitted = ImGui::InputText("要素名", session.aliasName.data(), session.aliasName.size(), ImGuiInputTextFlags_EnterReturnsTrue);
	if (submitted || ImGui::IsItemDeactivatedAfterEdit()) {
		const std::string name = session.aliasName.data();
		if (name != session.alias) {
			const bool success = manager.RenameElement(handle, session.alias, name);
			EditorToolbar::GetInstance().NotifyOperationResult("要素名の変更", success);
			if (success) {
				session.alias = name;
				return;
			}
		}
	}
	auto* node = manager.TryGetElement(handle, session.alias);
	if (node) {

		// 既存の詳細編集処理を同じInspectorへ埋め込み、別ウィンドウへの移動を不要化
		if (it->type == UIGroupElementType::Sprite) {
			DrawSpriteInspector(*static_cast<Sprite*>(node));
		} else if (it->type == UIGroupElementType::Text) {
			DrawTextInspector(*static_cast<Text*>(node));
		} else {
			DrawTransform2DInspector(*node);
			if (ImGui::Button("子Groupを編集")) {
				session.selected = it->group;
				session.alias.clear();
				return;
			}
		}
	} else {
		ImGui::TextColored({ 1.0f, 0.7f, 0.2f, 1.0f }, "未接続: 保存名・シーン・所属を確認してください");
	}
	ImGui::Separator();
	if (it->type == UIGroupElementType::Group) {
		ImGui::TextDisabled("子Groupの削除時は配下の要素を維持");
	}
	if (ImGui::Button("要素を削除")) {
		EditorToolbar::GetInstance().NotifyOperationResult("要素の削除", manager.DestroyElement(handle, session.alias));
		session.alias.clear();
	}
}

} // namespace

void SelectUIGroupEditorObject(UIGroupHandle handle, const std::string& elementName) {
	session.selected = handle;
	session.alias = elementName;
}

void DrawUIGroupEditorUI(SceneType currentSceneType) {
	auto& manager = UIGroupManager::GetInstance();
	auto& toolbar = EditorToolbar::GetInstance();
	manager.RefreshBindings();
	if (!manager.TryGet(session.selected) || !IsInScene(session.selected, currentSceneType)) {
		session.selected = {};
		session.alias.clear();
	}
	ImGui::SetNextWindowSize({ 760.0f, 560.0f }, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("UI Group Editor")) {
		ImGui::End();
		return;
	}
	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputText("新規名", session.createName.data(), session.createName.size());
	ImGui::Checkbox("全シーン共通", &session.global);
	ImGui::SameLine();
	if (ImGui::Button("Group追加")) {
		SceneType scene = currentSceneType;
		if (session.global) {
			scene = SceneType::None;
		}
		const auto created = manager.Create(session.createName.data(), scene);
		toolbar.NotifyOperationResult("Groupの追加", created.IsValid());
		if (created.IsValid()) {
			session.selected = created;
			session.alias.clear();
			const auto base = manager.GetName(created);
			uint32_t suffix = 1;
			while (manager.Find(base + std::to_string(suffix), scene).IsValid()) {
				++suffix;
			}
			CopyBuffer(session.createName, base + std::to_string(suffix));
		}
	}
	if (ImGui::Button("保存")) {
		const bool success = SaveUIGroupEditorJson(currentSceneType);
		toolbar.NotifyOperationResult("Group・Sprite・Textの保存", success);
		if (success) {
			toolbar.MarkDocumentSaved(EditorDocument::UIGroup);
			toolbar.MarkDocumentSaved(EditorDocument::Sprite);
			toolbar.MarkDocumentSaved(EditorDocument::Text);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("読込")) {
		toolbar.SuppressCurrentDocumentHistory();
		bool success = LoadSpriteEditorJson(currentSceneType);
		success = LoadTextEditorJson(currentSceneType) && success;
		success = LoadUIGroupEditorJson(currentSceneType) && success;
		toolbar.NotifyOperationResult("Group・Sprite・Textの読込", success);
		session.editing = {};
	}
	ImGui::Checkbox("原点をGame Viewに表示", &session.showOrigin);
	ImGui::Separator();
	const ImVec2 contentSize = ImGui::GetContentRegionAvail();
	const bool wideLayout = contentSize.x >= 600.0f;
	ImVec2 treeSize;
	if (wideLayout) {

		// ウィンドウを狭めたときは表示幅だけを制限し、Inspectorの最小幅を確保
		const float paneWidth = contentSize.x - kHierarchySplitterWidth;
		const float currentWidth = std::clamp(session.hierarchyWidth, kHierarchyMinWidth, paneWidth - kInspectorMinWidth);
		float hierarchyWidth = currentWidth;
		float inspectorWidth = paneWidth - currentWidth;

		// 一覧とInspectorの間に当たり判定を置き、描画前にドラッグ量を両方の幅へ反映
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const float splitterCenter = origin.x + currentWidth + kHierarchySplitterWidth * 0.5f;
		const ImRect splitter(splitterCenter - 1.0f, origin.y, splitterCenter + 1.0f, origin.y + (std::max)(4.0f, contentSize.y));
		ImGui::SplitterBehavior(splitter, ImGui::GetID("##HierarchySplitter"), ImGuiAxis_X,
			&hierarchyWidth, &inspectorWidth, kHierarchyMinWidth, kInspectorMinWidth, (kHierarchySplitterWidth - 2.0f) * 0.5f);
		if (hierarchyWidth != currentWidth) {
			session.hierarchyWidth = hierarchyWidth;
		}
		treeSize = { hierarchyWidth, 0.0f };
	} else {

		// 右側Dockの狭い幅でもInspectorを潰さないよう縦方向へ配置
		treeSize = { 0.0f, std::clamp(contentSize.y * 0.35f, 60.0f, 150.0f) };
	}
	ImGui::BeginChild("Hierarchy", treeSize, true);
	std::unordered_set<uint32_t> visited;
	for (auto handle : manager.GetHandles()) {
		if (IsInScene(handle, currentSceneType) && !manager.TryGet(handle)->HasParent()) {
			DrawGroupTree(handle, manager.GetName(handle) + " [" + SceneTypeToString(manager.GetSceneType(handle)) + "]", visited, 0);
		}
	}
	ImGui::EndChild();
	if (session.deleteOwner.IsValid()) {
		const bool success = manager.DestroyElement(session.deleteOwner, session.deleteAlias);
		toolbar.NotifyOperationResult("要素の削除", success);
		if (success) {

			// 選択中の子Groupや要素を削除した場合は親のInspectorへ戻り、無効参照を回避
			if (!manager.TryGet(session.selected)) {
				session.selected = session.deleteOwner;
				session.alias.clear();
			} else if (session.selected == session.deleteOwner && session.alias == session.deleteAlias) {
				session.alias.clear();
			}
		}
		session.deleteOwner = {};
		session.deleteAlias.clear();
	}
	if (wideLayout) {
		ImGui::SameLine(0.0f, kHierarchySplitterWidth);
	}
	ImGui::BeginChild("Inspector", { 0.0f, 0.0f }, true);
	if (auto* group = manager.TryGet(session.selected)) {

		// 入力中の文字列を毎フレーム上書きしないため、選択変更時だけバッファを同期
		if (session.editing != session.selected || session.editingAlias != session.alias ||
			session.displayedGroupName != manager.GetName(session.selected)) {
			CopyBuffer(session.groupName, manager.GetName(session.selected));
			CopyBuffer(session.aliasName, session.alias);
			session.editing = session.selected;
			session.editingAlias = session.alias;
			session.displayedGroupName = manager.GetName(session.selected);
		}
		if (!session.alias.empty()) {
			DrawElementInspector(session.selected);
		} else {
			const bool submitted = ImGui::InputText("Group名", session.groupName.data(), session.groupName.size(), ImGuiInputTextFlags_EnterReturnsTrue);
			if ((submitted || ImGui::IsItemDeactivatedAfterEdit()) && manager.GetName(session.selected) != session.groupName.data()) {
				toolbar.NotifyOperationResult("Group名の変更", manager.Rename(session.selected, session.groupName.data()));
			}
			ImGui::Text("シーン: %s", SceneTypeToString(manager.GetSceneType(session.selected)).c_str());
			DrawTransform2DInspector(*group);
			DrawCreation(session.selected);
			if (ImGui::CollapsingHeader("既存要素を登録")) {
				DrawRegistration(session.selected);
			}
			ImGui::SeparatorText("構成要素（選択して編集）");
			for (const auto& element : manager.GetElements(session.selected)) {
				ImGui::PushID(element.name.c_str());
				if (ImGui::Selectable(element.name.c_str())) {
					session.alias = element.name;
				}
				ImGui::PopID();
			}
			ImGui::Separator();
			if (ImGui::Button("Group削除（子要素を維持）")) {
				manager.Destroy(session.selected);
				session.selected = {};
			}
		}
	} else {
		ImGui::TextDisabled("Groupを追加または選択してください");
	}
	ImGui::EndChild();
	ImGui::End();
}

void DrawUIGroupOriginOverlay(const ImVec2& imageMin, const ImVec2& imageSize) {
	auto& manager = UIGroupManager::GetInstance();
	auto* node = manager.TryGet(session.selected);
	if (!session.showOrigin || !node || !EditorToolbar::GetInstance().IsWindowVisible(EditorWindow::UIGroup)) {
		return;
	}
	TransformNode2D* selected = node;
	if (!session.alias.empty()) {
		selected = manager.TryGetElement(session.selected, session.alias);
	}
	const auto size = SpriteManager::GetInstance().GetScreenSize();
	if (!selected || size.x <= 0.0f || size.y <= 0.0f || imageSize.x <= 0.0f || imageSize.y <= 0.0f) {
		return;
	}
	const auto matrix = selected->GetWorldMatrix();

	// 画面ピクセル座標を表示画像の座標へ変換し、余白と縮小表示を補正
	const auto project = [&](Vector3 point) {
		point = Matrix::Transform(point, matrix);
		return ImVec2(imageMin.x + point.x * imageSize.x / size.x, imageMin.y + point.y * imageSize.y / size.y);
	};
	const ImVec2 origin = project({});
	const ImVec2 x = project({ 20.0f, 0.0f, 0.0f });
	const ImVec2 y = project({ 0.0f, 20.0f, 0.0f });
	if (!std::isfinite(origin.x) || !std::isfinite(origin.y) ||
		!std::isfinite(x.x) || !std::isfinite(x.y) || !std::isfinite(y.x) || !std::isfinite(y.y)) {
		return;
	}
	auto* draw = ImGui::GetWindowDrawList();
	draw->PushClipRect(imageMin, { imageMin.x + imageSize.x, imageMin.y + imageSize.y }, true);
	draw->AddLine(origin, x, IM_COL32(255, 90, 90, 255), 2.0f);
	draw->AddLine(origin, y, IM_COL32(90, 255, 120, 255), 2.0f);
	draw->AddCircle(origin, 5.0f, IM_COL32(255, 225, 70, 255), 16, 2.0f);

	// 文字の高さと余白を確保して原点の上へ配置し、下側のUIとの重なりを回避
	const std::string label = manager.GetName(session.selected);
	const ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
	draw->AddText({ origin.x + 7.0f, origin.y - labelSize.y - 7.0f }, IM_COL32(255, 225, 70, 255), label.c_str());
	draw->PopClipRect();
}

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
