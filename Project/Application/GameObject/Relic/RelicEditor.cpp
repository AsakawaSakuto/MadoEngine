#include "RelicEditor.h"
#include "RelicSystem.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI
#include <algorithm>
#include <cstring>
#include <format>
#include <utility>

namespace {

#ifdef USE_IMGUI
constexpr std::array<const char*, static_cast<std::size_t>(Relic::Stat::Count)> kStatNames = {
	"与ダメージ",
	"最大HP",
	"移動速度",
	"全武器の最大発射数",
	"獲得Money",
	"HPリジェネ速度",
	"獲得EXP",
	"Projectileクリティカル率",
	"発射クールタイム",
	"ジャンプ可能回数",
	"被ダメージ",
};
constexpr std::array<const char*, 2> kOperationNames = {
	"固定加算",
	"割合加算",
};
constexpr std::array<const char*, static_cast<std::size_t>(Relic::PeriodicEffectType::Count)> kPeriodicEffectNames = {
	"Map上の全EXPを引き寄せる",
};
constexpr float kBasicFieldWidth = 420.0f;

/// @brief 一件のステータス補正を編集するImGuiを描画
/// @param effect 編集対象の効果
/// @param effectIndex レリック内の効果番号
/// @return 削除操作が選択された場合はtrue
bool DrawEffectEditor(Relic::StatModifierEffect& effect, std::size_t effectIndex) {
	ImGui::PushID(static_cast<int>(effectIndex));
	ImGui::SeparatorText(std::format("効果 {}", effectIndex + 1).c_str());

	int statIndex = static_cast<int>(effect.stat);
	ImGui::SetNextItemWidth(220.0f);
	if (ImGui::Combo("対象ステータス", &statIndex, kStatNames.data(), static_cast<int>(kStatNames.size()))) {
		effect.stat = static_cast<Relic::Stat>(statIndex);
	}

	int operationIndex = static_cast<int>(effect.operation);
	ImGui::SetNextItemWidth(220.0f);
	if (ImGui::Combo("演算", &operationIndex, kOperationNames.data(), static_cast<int>(kOperationNames.size()))) {
		effect.operation = static_cast<Relic::ModifierOperation>(operationIndex);
	}

	ImGui::SetNextItemWidth(220.0f);
	ImGui::DragFloat("1個あたりの値", &effect.valuePerStack, 0.01f, -999999.0f, 999999.0f, "%.3f");
	if (effect.operation == Relic::ModifierOperation::AddPercent) {
		ImGui::TextDisabled("0.10 = 10%%");
		if (effect.stat == Relic::Stat::IncomingDamage) {

			// 軽減は負の割合として設定し、他のステータスと共通の割合加算を維持
			ImGui::TextDisabled("-0.10 = 被ダメージ10%%軽減（所持数に応じて加算）");
		}
	}

	const bool shouldRemove = ImGui::Button("この効果を削除");
	ImGui::PopID();
	return shouldRemove;
}

/// @brief 一件の周期発動効果を編集するImGuiを描画
/// @param effect 編集対象の効果
/// @param effectIndex 周期効果内の効果番号
/// @param ownedCount 現在の所持数
/// @return 削除操作が選択された場合はtrue
bool DrawPeriodicEffectEditor(Relic::PeriodicEffect& effect, std::size_t effectIndex, int ownedCount) {
	ImGui::PushID(static_cast<int>(effectIndex));
	ImGui::SeparatorText(std::format("周期効果 {}", effectIndex + 1).c_str());
	int typeIndex = static_cast<int>(effect.type);
	ImGui::SetNextItemWidth(260.0f);
	if (ImGui::Combo("発動内容", &typeIndex, kPeriodicEffectNames.data(), static_cast<int>(kPeriodicEffectNames.size()))) {
		effect.type = static_cast<Relic::PeriodicEffectType>(typeIndex);
	}
	ImGui::SetNextItemWidth(220.0f);
	ImGui::DragFloat("発動間隔（秒）", &effect.intervalSeconds, 0.1f,
		Relic::PeriodicEffect::kMinIntervalSeconds, 3600.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);

	// 複数所持時の実効間隔を表示し、基礎秒数と短縮後の秒数の取り違えを防止
	ImGui::TextDisabled("実効間隔 = 設定秒数 / 所持数（最短0.1秒、ポーズ中は停止）");
	if (ownedCount > 0) {
		ImGui::Text("%d個所持時: %.2f秒ごと", ownedCount, effect.GetIntervalSeconds(ownedCount));
	}
	const bool shouldRemove = ImGui::Button("この周期効果を削除");
	ImGui::PopID();
	return shouldRemove;
}
#endif // USE_IMGUI

}

namespace Relic {

	void Editor::Initialize(const System& relicSystem) {
		editingDefinitions_ = relicSystem.GetDefinitions();
		selectedIndex_ = 0;
		SynchronizeTextBuffers();
	}

	void Editor::DrawImGui(System& relicSystem) {

#ifdef USE_IMGUI

		// 定義が存在しない場合も再読み込み操作だけを提供
		ImGui::Begin("レリック定義");
		if (editingDefinitions_.empty()) {
			ImGui::TextUnformatted("編集可能なレリック定義がありません");
			if (ImGui::Button("再読込")) {
				Reload(relicSystem);
			}
			ImGui::End();
			return;
		}

		// 編集対象の切り替え時に固定長テキストバッファも同期
		selectedIndex_ = (std::min)(selectedIndex_, editingDefinitions_.size() - 1);
		const char* selectedName = editingDefinitions_[selectedIndex_].displayName.c_str();
		ImGui::SetNextItemWidth(260.0f);
		if (ImGui::BeginCombo("調整するレリック", selectedName)) {
			for (std::size_t index = 0; index < editingDefinitions_.size(); ++index) {
				const bool isSelected = index == selectedIndex_;
				const std::string label = editingDefinitions_[index].displayName + "##" + editingDefinitions_[index].id;
				if (ImGui::Selectable(label.c_str(), isSelected)) {
					selectedIndex_ = index;
					SynchronizeTextBuffers();
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("保存して適用")) {
			ApplyTextBuffers();
			Save(relicSystem);
		}
		ImGui::SameLine();
		if (ImGui::Button("再読込")) {
			Reload(relicSystem);
		}
		ImGui::SameLine();
		if (ImGui::Button("新規追加")) {
			Definition definition;
			definition.id = CreateUniqueId();
			definition.displayName = "新規レリック";
			definition.description = "効果説明";
			definition.textureName = "None";
			definition.effects.push_back({});
			editingDefinitions_.push_back(std::move(definition));
			selectedIndex_ = editingDefinitions_.size() - 1;
			SynchronizeTextBuffers();
		}
		ImGui::SameLine();
		if (editingDefinitions_.size() <= 1) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("選択中を削除") && editingDefinitions_.size() > 1) {
			editingDefinitions_.erase(editingDefinitions_.begin() + static_cast<std::ptrdiff_t>(selectedIndex_));
			selectedIndex_ = (std::min)(selectedIndex_, editingDefinitions_.size() - 1);
			SynchronizeTextBuffers();
		}
		if (editingDefinitions_.size() <= 1) {
			ImGui::EndDisabled();
		}

		ImGui::SeparatorText("基本情報");
		bool textChanged = false;
		ImGui::SetNextItemWidth(kBasicFieldWidth);
		textChanged |= ImGui::InputText(
			"ID",
			idBuffer_.data(),
			idBuffer_.size(),
			ImGuiInputTextFlags_ReadOnly);
		ImGui::TextDisabled("所持数や保存データの照合に使う変更不可の内部識別子");
		ImGui::SetNextItemWidth(kBasicFieldWidth);
		textChanged |= ImGui::InputText("表示名", displayNameBuffer_.data(), displayNameBuffer_.size());
		textChanged |= ImGui::InputTextMultiline(
			"説明",
			descriptionBuffer_.data(),
			descriptionBuffer_.size(),
			ImVec2(kBasicFieldWidth, 64.0f));
		ImGui::SetNextItemWidth(kBasicFieldWidth);
		textChanged |= ImGui::InputText("画像名", textureNameBuffer_.data(), textureNameBuffer_.size());
		if (textChanged) {
			ApplyTextBuffers();
		}

		Definition& definition = editingDefinitions_[selectedIndex_];

		// 共通レアリティ定義を列挙して表示名と設定可能範囲を同期
		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::BeginCombo("固定レアリティ", GetRarityDisplayName(definition.rarity))) {
			for (const RarityDefinition& rarityDefinition : kRarityDefinitions) {
				const bool isSelected = definition.rarity == rarityDefinition.rarity;
				if (ImGui::Selectable(rarityDefinition.displayName, isSelected)) {
					definition.rarity = rarityDefinition.rarity;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::DragInt("所持上限", &definition.maxStack, 1.0f, 1, 999)) {
			definition.maxStack = std::clamp(definition.maxStack, 1, 999);
		}
		ImGui::SeparatorText("所持状態");
		const Definition* appliedDefinition = relicSystem.FindDefinition(definition.id);
		if (!appliedDefinition) {
			ImGui::TextColored(
				ImVec4(1.0f, 0.65f, 0.20f, 1.0f),
				"所持数を操作する前に定義を保存してください");
		} else {
			const int ownedCount = relicSystem.GetInventory().GetCount(appliedDefinition->id);
			ImGui::Text("現在の所持数: %d / %d", ownedCount, appliedDefinition->maxStack);

			const bool canAcquire = ownedCount < appliedDefinition->maxStack;
			if (!canAcquire) {
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("1個獲得") && canAcquire) {
				relicSystem.AcquireForEditor(appliedDefinition->id);
			}
			if (!canAcquire) {
				ImGui::EndDisabled();
			}

			ImGui::SameLine();
			const bool canDiscard = ownedCount > 0;
			if (!canDiscard) {
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("1個破棄") && canDiscard) {
				relicSystem.DiscardForEditor(appliedDefinition->id);
			}
			if (!canDiscard) {
				ImGui::EndDisabled();
			}
		}

		ImGui::SeparatorText("効果");
		for (std::size_t effectIndex = 0; effectIndex < definition.effects.size();) {
			if (DrawEffectEditor(definition.effects[effectIndex], effectIndex)) {
				definition.effects.erase(definition.effects.begin() + static_cast<std::ptrdiff_t>(effectIndex));
				continue;
			}
			++effectIndex;
		}
		if (ImGui::Button("効果を追加")) {
			definition.effects.push_back({});
		}

		// ステータス補正を持たない周期発動専用レリックも同じEditorで作成可能
		ImGui::SeparatorText("周期発動効果");
		ImGui::PushID("PeriodicEffects");
		const int ownedCount = relicSystem.GetInventory().GetCount(definition.id);
		for (std::size_t effectIndex = 0; effectIndex < definition.periodicEffects.size();) {
			if (DrawPeriodicEffectEditor(definition.periodicEffects[effectIndex], effectIndex, ownedCount)) {
				definition.periodicEffects.erase(definition.periodicEffects.begin() + static_cast<std::ptrdiff_t>(effectIndex));
				continue;
			}
			++effectIndex;
		}
		if (ImGui::Button("周期効果を追加")) {
			definition.periodicEffects.push_back({});
		}
		ImGui::PopID();
		if (definition.effects.empty() && definition.periodicEffects.empty()) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "効果を一件以上追加してください");
		}

		ImGui::End();

#else

		(void)relicSystem;

#endif // USE_IMGUI
	}

	bool Editor::Save(System& relicSystem) const {
		return relicSystem.SaveCatalog(editingDefinitions_);
	}

	bool Editor::Reload(System& relicSystem) {
		if (!relicSystem.ReloadCatalog()) {
			return false;
		}
		Initialize(relicSystem);
		return true;
	}

	void Editor::SynchronizeTextBuffers() {
		idBuffer_.fill('\0');
		displayNameBuffer_.fill('\0');
		descriptionBuffer_.fill('\0');
		textureNameBuffer_.fill('\0');
		if (editingDefinitions_.empty() || selectedIndex_ >= editingDefinitions_.size()) {
			return;
		}

		const Definition& definition = editingDefinitions_[selectedIndex_];
		strncpy_s(idBuffer_.data(), idBuffer_.size(), definition.id.c_str(), _TRUNCATE);
		strncpy_s(displayNameBuffer_.data(), displayNameBuffer_.size(), definition.displayName.c_str(), _TRUNCATE);
		strncpy_s(descriptionBuffer_.data(), descriptionBuffer_.size(), definition.description.c_str(), _TRUNCATE);
		strncpy_s(textureNameBuffer_.data(), textureNameBuffer_.size(), definition.textureName.c_str(), _TRUNCATE);
	}

	void Editor::ApplyTextBuffers() {
		if (editingDefinitions_.empty() || selectedIndex_ >= editingDefinitions_.size()) {
			return;
		}

		Definition& definition = editingDefinitions_[selectedIndex_];
		definition.id = idBuffer_.data();
		definition.displayName = displayNameBuffer_.data();
		definition.description = descriptionBuffer_.data();
		definition.textureName = textureNameBuffer_.data();
	}

	Id Editor::CreateUniqueId() const {
		for (std::size_t number = 1;; ++number) {
			const Id candidate = "relic_" + std::to_string(number);
			const bool isUsed = std::any_of(
				editingDefinitions_.begin(),
				editingDefinitions_.end(),
				[&candidate](const Definition& definition) {
					return definition.id == candidate;
				});
			if (!isUsed) {
				return candidate;
			}
		}
	}

}
