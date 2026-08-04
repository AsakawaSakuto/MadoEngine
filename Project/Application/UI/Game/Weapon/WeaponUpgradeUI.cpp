#include "WeaponUpgradeUI.h"
#include "GameObject/Weapon/WeaponInventory.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "Input/MyInput.h"
#include "ImGuiHeaders.h"
#include <algorithm>

namespace UI::Game {
	namespace {
		constexpr const char* kUpgradeLeftAction = "Left";
		constexpr const char* kUpgradeRightAction = "Right";
		constexpr const char* kUpgradeDecisionAction = "Decision";
	}

	void UpgradeUI::Initialize() {
		for (std::size_t cardIndex = 0; cardIndex < upgradeCards_.size(); ++cardIndex) {
			upgradeCards_[cardIndex].Initialize(cardIndex);
		}
		ResetSelection();

		MyInput::RegisterInput(kUpgradeLeftAction, { DIK_LEFT, DIK_A }, { GAMEPAD_LEFT });
		MyInput::RegisterInput(kUpgradeRightAction, { DIK_RIGHT, DIK_D }, { GAMEPAD_RIGHT });
		MyInput::RegisterInput(kUpgradeDecisionAction, { DIK_SPACE }, { GAMEPAD_A });
	}

	void UpgradeUI::Finalize() {
		for (WeaponUpgradeCardUI& card : upgradeCards_) {
			card.Finalize();
		}
		ResetSelection();
	}

	void UpgradeUI::Update(
		float deltaTime,
		Weapon::UpgradeSystem& upgradeSystem,
		Weapon::Inventory& inventory) {
		SynchronizeSelection(upgradeSystem);

		const std::vector<Weapon::UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (visibleChoiceCount_ == 0 || choices.empty()) {
			return;
		}

		if (MyInput::Trigger(kUpgradeLeftAction)) {
			selectedChoiceIndex_ =
				(selectedChoiceIndex_ + visibleChoiceCount_ - 1) % visibleChoiceCount_;
		} else if (MyInput::Trigger(kUpgradeRightAction)) {
			selectedChoiceIndex_ =
				(selectedChoiceIndex_ + 1) % visibleChoiceCount_;
		}

		UpdateCards(deltaTime);

		if (!MyInput::Trigger(kUpgradeDecisionAction)) {
			return;
		}

		const std::uint64_t generation = choices[selectedChoiceIndex_].generation;
		if (upgradeSystem.SelectChoice(selectedChoiceIndex_, generation, inventory)) {
			SynchronizeSelection(upgradeSystem);
			UpdateCards(0.0f);
		}
	}

	void UpgradeUI::DrawImGui(Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory) {
#ifdef USE_IMGUI
		SynchronizeSelection(upgradeSystem);

		ImGui::Begin("武器アップグレード");
		ImGui::Text("未処理アップグレード: %d", upgradeSystem.GetPendingUpgradeCount());
		ImGui::TextDisabled("↑ / W・↓ / S: 選択　Space / A: 決定");
		ImGui::Separator();

		const std::vector<Weapon::UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (choices.empty()) {
			ImGui::TextDisabled(upgradeSystem.IsUpgrading() ? "候補を生成できませんでした" : "レベルアップ待機中");
			ImGui::End();
			return;
		}

		for (std::size_t choiceIndex = 0; choiceIndex < choices.size(); ++choiceIndex) {
			const Weapon::UpgradeChoice& choice = choices[choiceIndex];
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

			if (choice.choiceType == Weapon::UpgradeChoiceType::OwnedWeaponUpgrade) {
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

	void UpgradeUI::SynchronizeSelection(const Weapon::UpgradeSystem& upgradeSystem) {
		if (!upgradeSystem.IsUpgrading()) {
			ResetSelection();
			return;
		}

		const std::vector<Weapon::UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (choices.empty()) {
			ResetSelection();
			return;
		}

		const std::uint64_t currentGeneration = choices.front().generation;
		if (selectedGeneration_ != currentGeneration) {
			selectedChoiceIndex_ = 0;
			selectedGeneration_ = currentGeneration;
			visibleChoiceCount_ = (std::min)(choices.size(), upgradeCards_.size());

			for (std::size_t cardIndex = 0; cardIndex < upgradeCards_.size(); ++cardIndex) {
				if (cardIndex < visibleChoiceCount_) {
					upgradeCards_[cardIndex].SetChoice(choices[cardIndex]);
				} else {
					upgradeCards_[cardIndex].SetVisible(false);
				}
			}
		}

		visibleChoiceCount_ = (std::min)(choices.size(), upgradeCards_.size());
		if (selectedChoiceIndex_ >= visibleChoiceCount_) {
			selectedChoiceIndex_ = 0;
		}
	}

	void UpgradeUI::UpdateCards(float deltaTime) {
		for (std::size_t cardIndex = 0; cardIndex < upgradeCards_.size(); ++cardIndex) {
			WeaponUpgradeCardUI& card = upgradeCards_[cardIndex];
			card.SetSelected(cardIndex < visibleChoiceCount_ && cardIndex == selectedChoiceIndex_);
			card.Update(deltaTime);
		}
	}

	void UpgradeUI::ResetSelection() {
		selectedChoiceIndex_ = 0;
		visibleChoiceCount_ = 0;
		selectedGeneration_ = 0;
		for (WeaponUpgradeCardUI& card : upgradeCards_) {
			card.SetSelected(false);
			card.SetVisible(false);
		}
	}
}
