#pragma once
#include "ImGuiHeaders.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace MadoEngine::Editor::Detail {

	/// @brief 文字列を固定長Bufferへコピーする
	/// @tparam Size Bufferの要素数
	/// @param buffer コピー先Buffer
	/// @param text コピー元文字列
	template<std::size_t Size>
	void CopyEmitterNameToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief 指定したEmitter名が使用済みか確認する
	/// @tparam TEmitter Emitter設定型
	/// @param emitters 確認対象一覧
	/// @param name 確認する名前
	/// @param ignoredIndex 確認対象から除外するIndex
	/// @return 使用済みの場合はtrue
	template<class TEmitter>
	bool IsEmitterNameUsed(
		const std::vector<TEmitter>& emitters,
		const std::string& name,
		int ignoredIndex = -1) {
		for (int index = 0; index < static_cast<int>(emitters.size()); ++index) {
			if (index != ignoredIndex && emitters[index].name == name) {
				return true;
			}
		}
		return false;
	}

	/// @brief Emitter一覧内で重複しない名前を生成する
	/// @tparam TEmitter Emitter設定型
	/// @param emitters 名前を確認する一覧
	/// @param createdName 初期名または複製元の名前
	/// @return 重複しない名前
	template<class TEmitter>
	std::string MakeUniqueEmitterName(
		const std::vector<TEmitter>& emitters,
		const std::string& createdName) {
		if (!IsEmitterNameUsed(emitters, createdName)) {
			return createdName;
		}

		std::size_t suffixStart = createdName.size();
		while (suffixStart > 0) {
			const char character = createdName[suffixStart - 1];
			if (character < '0' || character > '9') {
				break;
			}
			--suffixStart;
		}
		std::string baseName = createdName.substr(0, suffixStart);
		uint32_t suffix = 1;
		if (suffixStart < createdName.size()) {
			const char* begin = createdName.data() + suffixStart;
			const char* end = createdName.data() + createdName.size();
			const std::from_chars_result result = std::from_chars(begin, end, suffix);
			if (result.ec == std::errc{} && result.ptr == end) {
				++suffix;
			} else {
				baseName = createdName;
				suffix = 1;
			}
		}

		for (;; ++suffix) {
			const std::string candidate = baseName + std::to_string(suffix);
			if (!IsEmitterNameUsed(emitters, candidate)) {
				return candidate;
			}
		}
	}

	/// @brief Particle Editor形式のEmitter一覧Paneを描画する
	/// @tparam TEmitter Emitter設定型
	/// @param id ImGui ID
	/// @param assetName 編集中Asset名
	/// @param emitters 編集対象Emitter一覧
	/// @param maximumEmitterCount Emitter数上限
	/// @param selectedEmitterIndex 選択中Index
	/// @param newNameBuffer 新規Emitter名Buffer
	/// @param createAssetIdentity 新規名Bufferを同期したAsset名
	/// @param renameBuffer 名前編集Buffer
	/// @param renameIdentity Buffer同期用識別子
	/// @return Assetが変更された場合はtrue
	template<class TEmitter>
	bool DrawEffectEmitterListPane(
		const char* id,
		const std::string& assetName,
		std::vector<TEmitter>& emitters,
		std::size_t maximumEmitterCount,
		int& selectedEmitterIndex,
		std::array<char, 128>& newNameBuffer,
		std::string& createAssetIdentity,
		std::array<char, 128>& renameBuffer,
		std::string& renameIdentity) {
		bool changed = false;
		if (emitters.empty()) {
			emitters.push_back(TEmitter{});
			changed = true;
		}
		selectedEmitterIndex = std::clamp(
			selectedEmitterIndex,
			0,
			static_cast<int>(emitters.size()) - 1
		);

		ImGui::PushID(id);
		const float emitterListWidth = std::clamp(
			ImGui::GetContentRegionAvail().x * 0.28f,
			240.0f,
			300.0f
		);
		ImGui::BeginChild("EmitterListPane", ImVec2(emitterListWidth, 0.0f), true);
		ImGui::SeparatorText("エミッター一覧");
		ImGui::TextDisabled("%zu / %zu", emitters.size(), maximumEmitterCount);

		if (createAssetIdentity != assetName) {
			CopyEmitterNameToBuffer(newNameBuffer, MakeUniqueEmitterName(emitters, "Emitter"));
			createAssetIdentity = assetName;
		}
		ImGui::TextUnformatted("新規エミッター名");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##NewEmitterName", newNameBuffer.data(), newNameBuffer.size());
		const std::string newEmitterName = newNameBuffer.data();
		const bool isNewNameEmpty = newEmitterName.empty();
		const bool isNewNameDuplicated = IsEmitterNameUsed(emitters, newEmitterName);
		const bool reachedLimit = emitters.size() >= maximumEmitterCount;
		ImGui::BeginDisabled(reachedLimit || isNewNameEmpty || isNewNameDuplicated);
		if (ImGui::Button("追加", ImVec2(-FLT_MIN, 0.0f))) {
			TEmitter emitter;
			emitter.name = newEmitterName;
			emitters.push_back(std::move(emitter));
			selectedEmitterIndex = static_cast<int>(emitters.size()) - 1;
			renameIdentity.clear();
			CopyEmitterNameToBuffer(
				newNameBuffer,
				MakeUniqueEmitterName(emitters, newEmitterName)
			);
			changed = true;
		}
		ImGui::EndDisabled();
		if (reachedLimit) {
			ImGui::TextDisabled("エミッター数が上限に達しています。");
		} else if (isNewNameEmpty) {
			ImGui::TextDisabled("新規エミッター名を入力してください。");
		} else if (isNewNameDuplicated) {
			ImGui::TextDisabled("同じ名前のエミッターが存在します。");
		}

		ImGui::TextUnformatted("登録済みエミッター");
		const float emitterListHeight = std::clamp(
			static_cast<float>(emitters.size()) * ImGui::GetTextLineHeightWithSpacing() + 8.0f,
			96.0f,
			220.0f
		);
		ImGui::BeginChild("EmitterSelectionList", ImVec2(0.0f, emitterListHeight), true);
		for (int index = 0; index < static_cast<int>(emitters.size()); ++index) {
			ImGui::PushID(index);
			const bool isSelected = index == selectedEmitterIndex;
			if (ImGui::Checkbox("##EmitterEnabled", &emitters[index].isEnabled)) {
				changed = true;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("有効");
			}
			ImGui::SameLine();
			if (ImGui::Selectable(emitters[index].name.c_str(), isSelected)) {
				selectedEmitterIndex = index;
				renameIdentity.clear();
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		const float operationButtonWidth =
			(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
		ImGui::BeginDisabled(reachedLimit);
		if (ImGui::Button("複製", ImVec2(operationButtonWidth, 0.0f))) {
			TEmitter emitter = emitters[selectedEmitterIndex];
			emitter.name = MakeUniqueEmitterName(emitters, emitter.name);
			emitters.insert(emitters.begin() + selectedEmitterIndex + 1, std::move(emitter));
			++selectedEmitterIndex;
			renameIdentity.clear();
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(emitters.size() <= 1);
		if (ImGui::Button("削除", ImVec2(operationButtonWidth, 0.0f))) {
			ImGui::OpenPopup("エミッター削除確認");
		}
		ImGui::EndDisabled();

		if (ImGui::BeginPopupModal("エミッター削除確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("「%s」を削除しますか？", emitters[selectedEmitterIndex].name.c_str());
			ImGui::TextDisabled("保存するまでは再読み込みで元に戻せます。");
			if (ImGui::Button("削除する")) {
				emitters.erase(emitters.begin() + selectedEmitterIndex);
				selectedEmitterIndex = std::clamp(
					selectedEmitterIndex,
					0,
					static_cast<int>(emitters.size()) - 1
				);
				renameIdentity.clear();
				changed = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		TEmitter& selectedEmitter = emitters[selectedEmitterIndex];
		const std::string currentIdentity = assetName + "\n" +
			std::to_string(selectedEmitterIndex) + "\n" + selectedEmitter.name;
		if (renameIdentity != currentIdentity) {
			CopyEmitterNameToBuffer(renameBuffer, selectedEmitter.name);
			renameIdentity = currentIdentity;
		}
		ImGui::SeparatorText("名前変更");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##RenameEmitterName", renameBuffer.data(), renameBuffer.size());
		const std::string renameEmitterName = renameBuffer.data();
		const bool isRenameNameEmpty = renameEmitterName.empty();
		const bool isRenameNameDuplicated = IsEmitterNameUsed(
			emitters,
			renameEmitterName,
			selectedEmitterIndex
		);
		const bool isRenameNameChanged = renameEmitterName != selectedEmitter.name;
		ImGui::BeginDisabled(
			isRenameNameEmpty || isRenameNameDuplicated || !isRenameNameChanged
		);
		if (ImGui::Button("名前を変更", ImVec2(-FLT_MIN, 0.0f))) {
			selectedEmitter.name = renameEmitterName;
			renameIdentity = assetName + "\n" +
				std::to_string(selectedEmitterIndex) + "\n" + selectedEmitter.name;
			changed = true;
		}
		ImGui::EndDisabled();
		if (isRenameNameEmpty) {
			ImGui::TextDisabled("エミッター名を入力してください。");
		} else if (isRenameNameDuplicated) {
			ImGui::TextDisabled("同じ名前のエミッターが存在します。");
		}

		ImGui::EndChild();
		ImGui::PopID();
		return changed;
	}

} // namespace MadoEngine::Editor::Detail
