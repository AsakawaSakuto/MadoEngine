#pragma once
#include "ImGuiHeaders.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <string>
#include <vector>

namespace MadoEngine::Editor::Detail {

	/// @brief Effect Asset管理UIで要求された操作
	struct EffectAssetManagementActions {
		int selectedAssetIndex = 0;
		bool isSelectionChanged = false;
		bool isCreateRequested = false;
		bool isDuplicateRequested = false;
		bool isRenameRequested = false;
		bool isSaveRequested = false;
		bool isDeleteRequested = false;
		bool isLoadRequested = false;
	};

	/// @brief Effect Asset管理の共通レイアウトを描画
	/// @tparam NewNameSize 新規Asset名Buffer要素数
	/// @tparam RenameSize 変更名Buffer要素数
	/// @param id ImGui識別子
	/// @param assetNames 登録済みAsset名一覧
	/// @param selectedAssetIndex 選択中Asset Index
	/// @param newNameBuffer 新規Asset名Buffer
	/// @param renameBuffer 変更後Asset名Buffer
	/// @param isNewNameEmpty 新規Asset名が空の場合はtrue
	/// @param isNewNameAvailable 新規Asset名を使用できる場合はtrue
	/// @param isRenameNameEmpty 変更後Asset名が空の場合はtrue
	/// @param isRenameNameChanged Asset名が変更されている場合はtrue
	/// @param isRenameNameAvailable 変更後Asset名を使用できる場合はtrue
	/// @param isDirty 未保存の変更が存在する場合はtrue
	/// @return UIで要求された操作
	template<std::size_t NewNameSize, std::size_t RenameSize>
	EffectAssetManagementActions DrawEffectAssetManagement(
		const char* id,
		const std::vector<std::string>& assetNames,
		int selectedAssetIndex,
		std::array<char, NewNameSize>& newNameBuffer,
		std::array<char, RenameSize>& renameBuffer,
		bool isNewNameEmpty,
		bool isNewNameAvailable,
		bool isRenameNameEmpty,
		bool isRenameNameChanged,
		bool isRenameNameAvailable,
		bool isDirty) {
		EffectAssetManagementActions actions;
		actions.selectedAssetIndex = assetNames.empty()
			? 0
			: std::clamp(selectedAssetIndex, 0, static_cast<int>(assetNames.size()) - 1);

		ImGui::PushID(id);
		ImGui::TextUnformatted("新規アセット名");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##NewAssetName", newNameBuffer.data(), newNameBuffer.size());
		ImGui::BeginDisabled(isNewNameEmpty || !isNewNameAvailable);
		actions.isCreateRequested = ImGui::Button("新規作成");
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(
			assetNames.empty() || isNewNameEmpty || !isNewNameAvailable
		);
		actions.isDuplicateRequested = ImGui::Button("選択中を複製");
		ImGui::EndDisabled();
		if (isNewNameEmpty) {
			ImGui::TextDisabled("新規アセット名を入力してください。");
		} else if (!isNewNameAvailable) {
			ImGui::TextDisabled("同名のアセットが存在するか、使用できない文字が含まれています。");
		}

		ImGui::Separator();
		ImGui::TextUnformatted("アセット一覧");
		if (assetNames.empty()) {
			ImGui::TextDisabled("編集できるアセットがありません。");
			ImGui::PopID();
			return actions;
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo(
			"##AssetSelection",
			assetNames[actions.selectedAssetIndex].c_str())) {
			for (int index = 0; index < static_cast<int>(assetNames.size()); ++index) {
				const bool isSelected = index == actions.selectedAssetIndex;
				if (ImGui::Selectable(assetNames[index].c_str(), isSelected)) {
					actions.selectedAssetIndex = index;
					actions.isSelectionChanged = index != selectedAssetIndex;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		const float renameButtonWidth =
			ImGui::CalcTextSize("アセット名を変更").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float renameInputWidth = (std::max)(
			120.0f,
			ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - renameButtonWidth
		);
		ImGui::SetNextItemWidth(renameInputWidth);
		ImGui::InputText("##RenameAssetName", renameBuffer.data(), renameBuffer.size());
		ImGui::SameLine();
		ImGui::BeginDisabled(
			isRenameNameEmpty || !isRenameNameChanged || !isRenameNameAvailable
		);
		actions.isRenameRequested = ImGui::Button("アセット名を変更");
		ImGui::EndDisabled();
		if (isRenameNameEmpty) {
			ImGui::TextDisabled("変更後のアセット名を入力してください。");
		} else if (isRenameNameChanged && !isRenameNameAvailable) {
			ImGui::TextDisabled("変更後のアセット名は使用できません。");
		}

		actions.isSaveRequested = ImGui::Button("保存");
		ImGui::SameLine();
		if (ImGui::Button("削除")) {
			ImGui::OpenPopup("AssetDeleteConfirmation");
		}
		ImGui::SameLine();
		actions.isLoadRequested = ImGui::Button("読込");
		ImGui::SameLine();
		if (isDirty) {
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "未保存");
		} else {
			ImGui::TextDisabled("保存済み");
		}

		if (ImGui::BeginPopupModal(
			"AssetDeleteConfirmation",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("「%s」を削除しますか？", assetNames[actions.selectedAssetIndex].c_str());
			ImGui::TextDisabled("JSONファイルは.trashディレクトリへ退避されます。");
			if (ImGui::Button("削除する")) {
				actions.isDeleteRequested = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
		return actions;
	}

} // namespace MadoEngine::Editor::Detail
