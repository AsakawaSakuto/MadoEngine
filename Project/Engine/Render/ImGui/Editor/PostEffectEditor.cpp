#include "PostEffectEditor.h"

#ifdef USE_IMGUI

#include "Utility/Settings/PostEffectSettings.h"
#include "EditorToolbar.h"
#include "Render/PostEffect/PostEffectDefinitionRegistry.h"
#include "Utility/Json/JsonHeaders.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace MadoEngine::Editor {

namespace {

enum class PostEffectEditorOperationType {
	ChangeEffect,
	MovePass,
	LoadSettings,
	RestoreSnapshot,
	Count,
};

struct PostEffectEditorOperation {
	PostEffectEditorOperationType type = PostEffectEditorOperationType::ChangeEffect;
	Render::PostEffectPassHandle handle{};
	Render::PostEffectType effectType = Render::PostEffectType::CopyImage;
	Render::PostEffectPassScope scope = Render::PostEffectPassScope::Layer;
	std::size_t newIndex = 0;
	std::filesystem::path filePath;
	std::string snapshot;
};

/// @brief 次フレームへ予約されたEditor操作一覧を取得
/// @return 予約操作一覧
std::vector<PostEffectEditorOperation>& GetPendingPostEffectEditorOperations() {
	static std::vector<PostEffectEditorOperation> operations;
	return operations;
}

/// @brief Effect変更を次フレームへ予約
/// @param handle 変更対象のPass Handle
/// @param effectType 適用するEffect種別
void ReservePostEffectChange(Render::PostEffectPassHandle handle, Render::PostEffectType effectType) {
	PostEffectEditorOperation operation{};
	operation.type = PostEffectEditorOperationType::ChangeEffect;
	operation.handle = handle;
	operation.effectType = effectType;
	GetPendingPostEffectEditorOperations().push_back(operation);
}

/// @brief Pass並べ替えを次フレームへ予約
/// @param handle 移動対象のPass Handle
/// @param scope Passの適用先
/// @param newIndex 移動先index
void ReservePostEffectPassMove(
	Render::PostEffectPassHandle handle,
	Render::PostEffectPassScope scope,
	std::size_t newIndex) {
	PostEffectEditorOperation operation{};
	operation.type = PostEffectEditorOperationType::MovePass;
	operation.handle = handle;
	operation.scope = scope;
	operation.newIndex = newIndex;
	GetPendingPostEffectEditorOperations().push_back(operation);
}

/// @brief JSON設定読込を次フレームへ予約
/// @param filePath 読み込むJSONパス
void ReservePostEffectSettingsLoad(const std::filesystem::path& filePath) {
	PostEffectEditorOperation operation{};
	operation.type = PostEffectEditorOperationType::LoadSettings;
	operation.filePath = filePath;
	GetPendingPostEffectEditorOperations().push_back(std::move(operation));
}

/// @brief 重複しないPassキーを作成
/// @param manager 重複確認に使用するManager
/// @param prefix Passキーの接頭辞
/// @param nextId 次に試すID
/// @return 重複しないPassキー
std::string CreateUniquePostEffectPassKey(
	const Render::PostEffectManager& manager,
	const char* prefix,
	int& nextId) {
	std::string key;
	do {
		key = std::string(prefix) + std::to_string(nextId++);
	} while (manager.Find(key).IsValid());
	return key;
}

/// @brief EditorからPassを追加
/// @param manager 追加先のManager
/// @param scope 追加するPassの適用先
/// @return 追加したPassのHandle
Render::PostEffectPassHandle AddPostEffectPassFromEditor(
	Render::PostEffectManager& manager,
	Render::PostEffectPassScope scope) {
	static int nextLayerPassId = 1;
	static int nextScreenPassId = 1;
	if (scope == Render::PostEffectPassScope::Screen) {
		Render::ScreenPostEffectPassCreateDesc desc{};
		desc.key = CreateUniquePostEffectPassKey(manager, "ScreenEffectPass_", nextScreenPassId);
		desc.name = "Screen Effect " + std::to_string(nextScreenPassId - 1);
		return manager.CreateScreenPass(desc);
	}

	Render::LayerPostEffectPassCreateDesc desc{};
	desc.key = CreateUniquePostEffectPassKey(manager, "LayerEffectPass_", nextLayerPassId);
	desc.name = "Layer Effect " + std::to_string(nextLayerPassId - 1);
	return manager.CreateLayerPass(desc);
}

/// @brief 対象Layerの選択Comboを描画
/// @param pass 編集対象のPass
void DrawLayerSelectionCombo(Render::PostEffectPass& pass) {
	const Render::RenderLayerMask currentLayerMask = pass.GetTargetLayerMask();
	ImGui::SetNextItemWidth(-1.0f);
	if (!ImGui::BeginCombo("##TargetLayer", Render::GetRenderLayerMaskName(currentLayerMask))) {
		return;
	}

	for (uint32_t index = 0; index < Render::kRenderLayerCount; ++index) {
		const Render::RenderLayer layer = Render::GetRenderLayerByIndex(index);
		const Render::RenderLayerMask layerMask = Render::ToRenderLayerMask(layer);
		const bool selected = layerMask == currentLayerMask;
		if (ImGui::Selectable(Render::GetRenderLayerName(layer), selected)) {
			pass.SetTargetLayerMask(layerMask);
		}
		if (selected) {
			ImGui::SetItemDefaultFocus();
		}
	}

	ImGui::EndCombo();
}

/// @brief Effect選択Comboを描画
/// @param handle 編集対象のPass Handle
/// @param pass 編集対象の一時参照
void DrawPostEffectSelectionCombo(
	Render::PostEffectPassHandle handle,
	const Render::PostEffectPass& pass) {
	const Render::PostEffectDefinition* currentDefinition = Detail::FindPostEffectDefinition(pass);
	const char* preview = [&]() {
		if (currentDefinition) {
			return (currentDefinition->displayName.data());
		} else {
			return ("Unknown");
		}
	}();
	ImGui::SetNextItemWidth(-1.0f);
	if (!ImGui::BeginCombo("##PostEffect", preview)) {
		return;
	}

	for (const Render::PostEffectDefinition& definition : Render::PostEffectDefinitionRegistry::GetAll()) {
		const bool selected = currentDefinition && definition.type == currentDefinition->type;
		if (ImGui::Selectable(definition.displayName.data(), selected)) {
			ReservePostEffectChange(handle, definition.type);
		}
		if (selected) {
			ImGui::SetItemDefaultFocus();
		}
	}
	ImGui::EndCombo();
}

/// @brief パラメータkeyが指定文字で終わるか判定
/// @param value 判定する文字列
/// @param suffix 末尾文字
/// @return 指定文字で終わる場合はtrue
bool EndsWith(std::string_view value, char suffix) {
	return !value.empty() && value.back() == suffix;
}

/// @brief 色成分keyから共通部分を取得
/// @param value 色成分key
/// @return 末尾のRGBAを除いたkey
std::string GetColorBase(std::string_view value) {
	if (EndsWith(value, 'R') || EndsWith(value, 'G') || EndsWith(value, 'B') || EndsWith(value, 'A')) {
		return std::string(value.substr(0, value.size() - 1));
	}
	return std::string(value);
}

/// @brief Registry定義から色編集可能な連続成分数を取得
/// @param parameters Parameter定義一覧
/// @param startIndex 確認開始index
/// @return RGBなら3、RGBAなら4、色でない場合は0
int GetColorComponentCount(
	std::span<const Render::PostEffectFloatParameterDefinition> parameters,
	std::size_t startIndex) {
	if (startIndex + 2 >= parameters.size() || !EndsWith(parameters[startIndex].key, 'R')) {
		return 0;
	}

	const std::string base = GetColorBase(parameters[startIndex].key);
	const std::size_t offset = parameters[startIndex].offset;
	for (std::size_t component = 1; component < 3; ++component) {
		const char suffix = [&]() {
			if (component == 1) {
				return ('G');
			} else {
				return ('B');
			}
		}();
		if (parameters[startIndex + component].key != base + suffix ||
			parameters[startIndex + component].offset != offset + sizeof(float) * component) {
			return 0;
		}
	}

	if (startIndex + 3 < parameters.size() &&
		parameters[startIndex + 3].key == base + 'A' &&
		parameters[startIndex + 3].offset == offset + sizeof(float) * 3) {
		return 4;
	}
	return 3;
}

/// @brief Registry定義を使って型非依存のParameter編集UIを描画
/// @param pass 編集対象のPass
void DrawPostEffectParameterRows(Render::PostEffectPass& pass) {
	const Render::PostEffectDefinition* definition = Detail::FindPostEffectDefinition(pass);
	if (!definition || definition->parameterCount == 0) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::TextDisabled("調整項目はありません");
		return;
	}

	const std::span<const Render::PostEffectFloatParameterDefinition> parameters = definition->GetParameters();
	for (std::size_t index = 0; index < parameters.size(); ++index) {
		const Render::PostEffectFloatParameterDefinition& parameter = parameters[index];
		const int colorComponentCount = GetColorComponentCount(parameters, index);
		if (colorComponentCount > 0) {
			float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			bool readable = true;
			for (int component = 0; component < colorComponentCount; ++component) {
				readable &= pass.TryGetFloatParameter(parameters[index + component].offset, color[component]);
			}
			if (readable) {
				std::string label(parameter.label);
				if (!label.empty()) {
					label.pop_back();
				}
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(label.c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::SetNextItemWidth(-1.0f);
				const std::string id = "##" + GetColorBase(parameter.key);
				const bool changed = [&]() {
					if (colorComponentCount == 4) {
						return (ImGui::ColorEdit4(id.c_str(), color));
					} else {
						return (ImGui::ColorEdit3(id.c_str(), color));
					}
				}();
				if (changed) {
					for (int component = 0; component < colorComponentCount; ++component) {
						pass.SetFloatParameter(parameters[index + component].offset, color[component]);
					}
				}
			}
			index += static_cast<std::size_t>(colorComponentCount - 1);
			continue;
		}

		float value = 0.0f;
		if (!pass.TryGetFloatParameter(parameter.offset, value)) {
			continue;
		}
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(parameter.label.data());
		ImGui::TableSetColumnIndex(2);
		ImGui::SetNextItemWidth(-1.0f);
		const std::string id = "##" + std::string(parameter.key);
		const std::span<const std::string_view> selectionOptions = parameter.GetSelectionOptions();
		if (!selectionOptions.empty()) {

			// Shaderへ渡すfloat値を整数Indexとして扱い選択肢以外の値をEditorから生成しない制約
			const int selectedIndex = std::clamp(
				static_cast<int>(std::round(value)),
				0,
				static_cast<int>(selectionOptions.size() - 1)
			);
			if (ImGui::BeginCombo(id.c_str(), selectionOptions[selectedIndex].data())) {
				for (std::size_t optionIndex = 0; optionIndex < selectionOptions.size(); ++optionIndex) {
					const bool selected = static_cast<int>(optionIndex) == selectedIndex;
					if (ImGui::Selectable(selectionOptions[optionIndex].data(), selected)) {
						pass.SetFloatParameter(parameter.offset, static_cast<float>(optionIndex));
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			continue;
		}
		if (ImGui::DragFloat(id.c_str(), &value, parameter.speed, parameter.minValue, parameter.maxValue)) {
			pass.SetFloatParameter(parameter.offset, value);
		}
	}
}

/// @brief Handleの現在の実行順indexを取得
/// @param handles 検索対象のHandle順序
/// @param handle 検索するHandle
/// @return 見つかったindex、存在しない場合はhandles.size()
std::size_t FindHandleIndex(
	const std::vector<Render::PostEffectPassHandle>& handles,
	Render::PostEffectPassHandle handle) {
	const auto it = std::find(handles.begin(), handles.end(), handle);
	if (it == handles.end()) {
		return handles.size();
	} else {
		return static_cast<std::size_t>(std::distance(handles.begin(), it));
	}
}

} // namespace

void ReservePostEffectEditorStateRestore(const std::string& snapshot) {
	PostEffectEditorOperation operation{};
	operation.type = PostEffectEditorOperationType::RestoreSnapshot;
	operation.snapshot = snapshot;
	GetPendingPostEffectEditorOperations().push_back(std::move(operation));
}

void ApplyPendingPostEffectEditorOperations(Render::PostEffectManager& postEffectManager) {
	std::vector<PostEffectEditorOperation>& pending = GetPendingPostEffectEditorOperations();
	if (pending.empty()) {
		return;
	}

	std::vector<PostEffectEditorOperation> operations = std::move(pending);
	pending.clear();
	bool hasHistoryChange = false;
	bool hasSettingsLoad = false;
	bool hasSnapshotRestore = false;
	for (const PostEffectEditorOperation& operation : operations) {
		switch (operation.type) {
		case PostEffectEditorOperationType::ChangeEffect:
			if (postEffectManager.IsValid(operation.handle)) {
				postEffectManager.SetEffectType(operation.handle, operation.effectType);
				hasHistoryChange = true;
			}
			break;
		case PostEffectEditorOperationType::MovePass:
			if (operation.scope == Render::PostEffectPassScope::Layer) {
				postEffectManager.MoveLayerPass(operation.handle, operation.newIndex);
				hasHistoryChange = true;
			} else if (operation.scope == Render::PostEffectPassScope::Screen) {
				postEffectManager.MoveScreenPass(operation.handle, operation.newIndex);
				hasHistoryChange = true;
			}
			break;
		case PostEffectEditorOperationType::LoadSettings:
			hasSettingsLoad =
				LoadPostEffectEditorJsonFromFile(postEffectManager, operation.filePath) || hasSettingsLoad;
			break;
		case PostEffectEditorOperationType::RestoreSnapshot:
			hasSnapshotRestore =
				RestorePostEffectEditorState(postEffectManager, operation.snapshot) || hasSnapshotRestore;
			break;
		case PostEffectEditorOperationType::Count:
		default:
			break;
		}
	}

	EditorToolbar& toolbar = EditorToolbar::GetInstance();
	if (hasSettingsLoad) {

		// JSON読込を履歴境界として既存履歴を破棄しSnapshotだけを同期
		toolbar.ClearHistory();
		toolbar.SynchronizeHistorySnapshots();
	} else if (hasSnapshotRestore) {
		toolbar.SynchronizeHistorySnapshots();
	} else if (hasHistoryChange) {
		toolbar.CommitExternalDocumentChange(EditorDocument::PostEffect);
	}
}

void DrawPostEffectEditorUI(Render::PostEffectManager& postEffectManager) {
	static Render::PostEffectPassHandle selectedHandle{};

	ImGui::Begin("Post Effect Editor");
	if (ImGui::Button("レイヤー追加")) {
		selectedHandle = AddPostEffectPassFromEditor(postEffectManager, Render::PostEffectPassScope::Layer);
	}
	ImGui::SameLine();
	if (ImGui::Button("フルスクリーン追加")) {
		selectedHandle = AddPostEffectPassFromEditor(postEffectManager, Render::PostEffectPassScope::Screen);
	}

	if (ImGui::Button("保存")) {
		SavePostEffectEditorJsonToFile(postEffectManager);
	}
	ImGui::SameLine();
	if (ImGui::Button("読込")) {
		EditorToolbar::GetInstance().SuppressCurrentDocumentHistory();
		ReservePostEffectSettingsLoad(kPostEffectEditorJsonPath);
		selectedHandle = {};
	}
	ImGui::SameLine();
	if (ImGui::Button("復元")) {
		EditorToolbar::GetInstance().SuppressCurrentDocumentHistory();
		ReservePostEffectSettingsLoad(Detail::CreateBackupJsonPath(kPostEffectEditorJsonPath));
		selectedHandle = {};
	}

	ImGui::Separator();
	Render::PostEffectPassHandle removeRequest{};
	ImGui::BeginChild("PostEffectPassList", ImVec2(260.0f, 0.0f), true);
	auto drawList = [&](const std::vector<Render::PostEffectPassHandle>& handles, const char* scopeLabel) {
		for (Render::PostEffectPassHandle handle : handles) {
			Render::PostEffectPass* pass = postEffectManager.TryGet(handle);
			if (!pass) {
				continue;
			}

			ImGui::PushID(static_cast<int>(handle.index));
			bool enabled = pass->IsEnabled();
			if (ImGui::Checkbox("##Enabled", &enabled)) {
				pass->SetEnabled(enabled);
			}
			ImGui::SameLine();

			const std::string label = std::string("[") + scopeLabel + "] " + pass->GetName();
			const float deleteButtonWidth =
				ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float selectableWidth =
				ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x;
			if (selectableWidth < 1.0f) {
				selectableWidth = 1.0f;
			}
			if (ImGui::Selectable(
				label.c_str(),
				selectedHandle == handle,
				0,
				ImVec2(selectableWidth, 0.0f))) {
				selectedHandle = handle;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("削除")) {
				removeRequest = handle;
			}
			ImGui::PopID();
		}
	};
	drawList(postEffectManager.GetLayerPassHandles(), "Layer");
	drawList(postEffectManager.GetScreenPassHandles(), "Screen");
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("PostEffectPassProperties", ImVec2(0.0f, 0.0f), true);
	Render::PostEffectPass* selectedPass = postEffectManager.TryGet(selectedHandle);
	if (!selectedPass) {
		selectedHandle = {};
		ImGui::TextDisabled("PostEffectPassを選択してください。");
	} else {
		ImGui::PushID(static_cast<int>(selectedHandle.index));
		std::array<char, 128> nameBuffer{};
		std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", selectedPass->GetName().c_str());
		if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size()) && nameBuffer[0] != '\0') {
			selectedPass->SetName(nameBuffer.data());
		}
		ImGui::Text("Key: %s", selectedPass->GetKey().c_str());

		ImGui::TextUnformatted("Effect");
		DrawPostEffectSelectionCombo(selectedHandle, *selectedPass);

		const Render::PostEffectPassScope scope = postEffectManager.GetScope(selectedHandle);
		if (scope == Render::PostEffectPassScope::Layer) {
			ImGui::TextUnformatted("Layer");
			DrawLayerSelectionCombo(*selectedPass);
			int stageIndex = static_cast<int>(selectedPass->GetLayerEffectStage());
			const char* stages[] = { "Scene", "Transparent", "Overlay" };
			if (ImGui::Combo("Stage", &stageIndex, stages, 3)) {
				selectedPass->SetLayerEffectStage(static_cast<Render::LayerEffectStage>(stageIndex));
			}
			bool ignoreDepth = selectedPass->IsIgnoreDepthForMask();
			if (ImGui::Checkbox("Ignore Depth", &ignoreDepth)) {
				selectedPass->SetIgnoreDepthForMask(ignoreDepth);
			}
		} else {
			int stageIndex = static_cast<int>(selectedPass->GetScreenEffectStage());
			const char* stages[] = { "Scene", "Final" };
			if (ImGui::Combo("Stage", &stageIndex, stages, 2)) {
				selectedPass->SetScreenEffectStage(static_cast<Render::ScreenEffectStage>(stageIndex));
			}
		}

		const std::vector<Render::PostEffectPassHandle>& order =
			[&]() {
				if (scope == Render::PostEffectPassScope::Layer) {
					return (postEffectManager.GetLayerPassHandles());
				} else {
					return (postEffectManager.GetScreenPassHandles());
				}
			}();
		const std::size_t currentIndex = FindHandleIndex(order, selectedHandle);
		ImGui::BeginDisabled(currentIndex == 0 || currentIndex >= order.size());
		if (ImGui::Button("上へ")) {
			ReservePostEffectPassMove(selectedHandle, scope, currentIndex - 1);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(currentIndex + 1 >= order.size());
		if (ImGui::Button("下へ")) {
			ReservePostEffectPassMove(selectedHandle, scope, currentIndex + 1);
		}
		ImGui::EndDisabled();

		ImGui::SeparatorText("Parameters");
		constexpr ImGuiTableFlags propertyTableFlags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("PostEffectPassPropertyTable", 3, propertyTableFlags)) {
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 1.0f);
			ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch);
			DrawPostEffectParameterRows(*selectedPass);
			ImGui::EndTable();
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	if (removeRequest.IsValid()) {

		// GPU ResourceをPostDrawの完了待機後まで保持して次Frame開始前に破棄
		postEffectManager.RequestDestroy(removeRequest);
		if (selectedHandle == removeRequest) {
			selectedHandle = {};
		}
	}
	ImGui::End();
}

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
