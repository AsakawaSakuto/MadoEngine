#include "WeaponUpgradeUI.h"
#include "GameObject/Weapon/WeaponInventory.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "Input/MyInput.h"
#include "ImGuiHeaders.h"

namespace Weapon {
	namespace {
		constexpr const char* kUpgradeUpAction = "Up";
		constexpr const char* kUpgradeDownAction = "Down";
		constexpr const char* kUpgradeDecisionAction = "Decision";
	}

	void UpgradeUI::Initialize() {
		ResetSelection();

		MyInput::RegisterInput(kUpgradeUpAction, { DIK_UP, DIK_W }, { GAMEPAD_UP });
		MyInput::RegisterInput(kUpgradeDownAction, { DIK_DOWN, DIK_S }, { GAMEPAD_DOWN });
		MyInput::RegisterInput(kUpgradeDecisionAction, { DIK_SPACE }, { GAMEPAD_A });
	}

	void UpgradeUI::UpdateInput(UpgradeSystem& upgradeSystem, Inventory& inventory) {
		SynchronizeSelection(upgradeSystem);

		const std::vector<UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (choices.empty()) {
			return;
		}

		if (MyInput::Trigger(kUpgradeUpAction)) {
			selectedChoiceIndex_ =
				(selectedChoiceIndex_ + choices.size() - 1) % choices.size();
		} else if (MyInput::Trigger(kUpgradeDownAction)) {
			selectedChoiceIndex_ =
				(selectedChoiceIndex_ + 1) % choices.size();
		}

		if (!MyInput::Trigger(kUpgradeDecisionAction)) {
			return;
		}

		const std::uint64_t generation = choices[selectedChoiceIndex_].generation;
		if (upgradeSystem.SelectChoice(selectedChoiceIndex_, generation, inventory)) {
			ResetSelection();
		}
	}

	void UpgradeUI::DrawImGui(UpgradeSystem& upgradeSystem, Inventory& inventory) {
#ifdef USE_IMGUI
		SynchronizeSelection(upgradeSystem);

		ImGui::Begin("武器アップグレード");
		ImGui::Text("未処理アップグレード: %d", upgradeSystem.GetPendingUpgradeCount());
		ImGui::TextDisabled("↑ / W・↓ / S: 選択　Space / A: 決定");
		ImGui::Separator();

		const std::vector<UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (choices.empty()) {
			ImGui::TextDisabled(upgradeSystem.IsUpgrading() ? "候補を生成できませんでした" : "レベルアップ待機中");
			ImGui::End();
			return;
		}

		for (std::size_t choiceIndex = 0; choiceIndex < choices.size(); ++choiceIndex) {
			const UpgradeChoice& choice = choices[choiceIndex];
			ImGui::PushID(static_cast<int>(choiceIndex));
			if (choiceIndex == selectedChoiceIndex_) {
				ImGui::TextColored(
					ImVec4(1.0f, 0.85f, 0.15f, 1.0f),
					"▶ 選択中 %zu: %s",
					choiceIndex + 1,
					choice.weaponDisplayName.c_str()
				);
			} else {
				ImGui::Text("候補 %zu: %s", choiceIndex + 1, choice.weaponDisplayName.c_str());
			}
			ImGui::Text("内容: %s", choice.choiceTypeDisplayName.c_str());

			if (choice.choiceType == UpgradeChoiceType::OwnedWeaponUpgrade) {
				const auto& color = choice.rarityDisplayColor;
				ImGui::TextColored(
					ImVec4(color[0], color[1], color[2], color[3]),
					"レアリティ: %s",
					choice.rarityDisplayName.c_str()
				);
				ImGui::Text("強化ステータス: %s", choice.statDisplayName.c_str());
				ImGui::Text("加算値: %+.3f", choice.calculatedAmount);
			} else {
				ImGui::TextDisabled("強化ステータス・レアリティなし");
			}

			const std::uint64_t generation = choice.generation;
			if (ImGui::Button("この候補を選択")) {
				selectedChoiceIndex_ = choiceIndex;
				if (upgradeSystem.SelectChoice(choiceIndex, generation, inventory)) {
					ResetSelection();
				}
				ImGui::PopID();
				break;
			}

			ImGui::Separator();
			ImGui::PopID();
		}

		ImGui::End();
#else
		(void)upgradeSystem;
		(void)inventory;
#endif // USE_IMGUI
	}

	void UpgradeUI::SynchronizeSelection(const UpgradeSystem& upgradeSystem) {
		if (!upgradeSystem.IsUpgrading()) {
			ResetSelection();
			return;
		}

		const std::vector<UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (choices.empty()) {
			ResetSelection();
			return;
		}

		const std::uint64_t currentGeneration = choices.front().generation;
		if (selectedGeneration_ != currentGeneration) {
			selectedChoiceIndex_ = 0;
			selectedGeneration_ = currentGeneration;
		}

		if (selectedChoiceIndex_ >= choices.size()) {
			selectedChoiceIndex_ = 0;
		}
	}

	void UpgradeUI::ResetSelection() {
		selectedChoiceIndex_ = 0;
		selectedGeneration_ = 0;
	}
}
