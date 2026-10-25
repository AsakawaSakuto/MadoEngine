#include "WeaponUpgradeUI.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Weapon/WeaponInventory.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "Input/MyInput.h"
#include "ImGuiHeaders.h"
#include <algorithm>

namespace UI::Game {
	namespace {
		constexpr const char* kUpgradeCardSelectSoundKey = "UpgradeCardSelect";
		constexpr const char* kUpgradeCardDecisionSoundKey = "UpgradeCardDecision";
		constexpr float kDecisionInputDelayDuration = 0.25f;
		constexpr const char* kUpgradeStickInputName = "WeaponUpgradeSelection";
		constexpr Vector2 kOverlayPosition = { 640.0f, 388.0f };
		constexpr Vector2 kOverlayScale = { 80.0f, 44.5f };
		constexpr Vector4 kOverlayColor = { 0.0f, 0.0f, 0.0f, 184.0f / 255.0f };
	}

	void UpgradeUI::Initialize() {
		MyInput::RegisterStickHorizontalInput(kUpgradeStickInputName);
		const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;
		overlaySprite_ = MySprite::Create("WeaponUpgradeOverlay","white16x16",SceneType::Game,layer);
		if (Sprite* overlay = MySprite::TryGet(overlaySprite_)) {
			overlay->SetAnchorPoint({ 0.5f, 0.5f });
			overlay->SetPosition(kOverlayPosition);
			overlay->SetScale(kOverlayScale);
			overlay->SetColor(kOverlayColor);
			overlay->SetVisible(false);
		}

		// 暗幕の後にCardを生成して既存HUDと選択肢の間へ描画順を固定
		for (std::size_t cardIndex = 0; cardIndex < upgradeCards_.size(); ++cardIndex) {
			upgradeCards_[cardIndex].Initialize(cardIndex);
		}
		ResetSelection();
	}

	void UpgradeUI::Finalize() {
		ResetSelection();
		MyInput::RemoveStickHorizontalInput(kUpgradeStickInputName);
		for (WeaponUpgradeCardUI& card : upgradeCards_) {
			card.Finalize();
		}
		overlaySprite_ = {};
	}

	void UpgradeUI::Update(
		float deltaTime,
		Weapon::UpgradeSystem& upgradeSystem,
		Weapon::Inventory& inventory,
		Player::Base& player) {

		// 表示開始Frameの経過時間を含めず、Game進行の停止中も実時間で決定待機を進行
		decisionInputDelayTimer_.Update(deltaTime);
		SynchronizeSelection(upgradeSystem);

		const std::vector<Weapon::UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (visibleChoiceCount_ == 0 || choices.empty()) {
			return;
		}

		if (isDecisionAnimationPlaying_) {

			// 演出中に候補世代が変化した場合は古い選択の適用を中止
			UpdateCards(deltaTime);
			const bool isChoiceValid =
				decisionChoiceIndex_ < visibleChoiceCount_ &&
				decisionChoiceIndex_ < choices.size() &&
				choices[decisionChoiceIndex_].generation == decisionGeneration_;
			if (!isChoiceValid) {
				ResetDecisionAnimationState();
				return;
			}
			if (!upgradeCards_[decisionChoiceIndex_].IsDecisionAnimationFinished()) {
				return;
			}

			// 決定演出完了後に保持した世代番号付き候補を適用
			const std::size_t choiceIndex = decisionChoiceIndex_;
			const std::uint64_t generation = decisionGeneration_;
			ResetDecisionAnimationState();
			if (upgradeSystem.SelectChoice(choiceIndex, generation, inventory)) {
				SynchronizeSelection(upgradeSystem);
				UpdateCards(0.0f);
			}
			return;
		}

		if (MyInput::GetKeyboard()->IsTrigger(DIK_F5)) {
			if (upgradeSystem.RerollChoicesForDebug(inventory)) {
				SynchronizeSelection(upgradeSystem);
				UpdateCards(0.0f);
			}
			return;
		}

		if (MyInput::Trigger("Reroll")) {
			if (upgradeSystem.RerollChoices(inventory, player)) {
				SynchronizeSelection(upgradeSystem);
				UpdateCards(0.0f);
			}
			return;
		}

		const int stickSelectionInput = MyInput::GetStickHorizontalTrigger(kUpgradeStickInputName);
		if (MyInput::Trigger("Left") || stickSelectionInput < 0) {

			// 端から反対側へ循環する候補選択
			selectedChoiceIndex_ =
				(selectedChoiceIndex_ + visibleChoiceCount_ - 1) % visibleChoiceCount_;

			MyAudio::Play(kUpgradeCardSelectSoundKey);

		} else if (MyInput::Trigger("Right") || stickSelectionInput > 0) {
			selectedChoiceIndex_ =
				(selectedChoiceIndex_ + 1) % visibleChoiceCount_;

			MyAudio::Play(kUpgradeCardSelectSoundKey);
		}

		UpdateCards(deltaTime);

		// 表示直後の決定入力を持ち越さず、待機完了後の新しい入力だけを受付
		if (!decisionInputDelayTimer_.IsFinished() || !MyInput::Trigger("Decision")) {
			return;
		} else {
			MyAudio::Play(kUpgradeCardDecisionSoundKey);
		}

		BeginDecisionAnimation(
			selectedChoiceIndex_,
			choices[selectedChoiceIndex_].generation);
	}

	void UpgradeUI::DrawImGui(Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory) {
#ifdef USE_IMGUI
		(void)inventory;
		SynchronizeSelection(upgradeSystem);

		ImGui::Begin("武器アップグレード");
		ImGui::Text("未処理アップグレード: %d", upgradeSystem.GetPendingUpgradeCount());
		ImGui::Text("次回リロール料金: %d", upgradeSystem.GetRerollCost());
		ImGui::TextDisabled("← / A・→ / D・Lスティック左右: 選択　Space / A: 決定　R / X: リロール　F5: 無料リロール(Debug)");
		if (isDecisionAnimationPlaying_) {
			ImGui::TextDisabled("選択決定演出中");
		}
		ImGui::Separator();

		const std::vector<Weapon::UpgradeChoice>& choices = upgradeSystem.GetChoices();
		if (choices.empty()) {
			ImGui::TextDisabled([&]() {
				if (upgradeSystem.IsUpgrading()) {
					return ("候補を生成できませんでした");
				} else {
					return ("レベルアップ待機中");
				}
			}());
			ImGui::End();
			return;
		}

		// Runtimeと同じ選択状態を維持したまま候補詳細とDebug決定操作を表示
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
				if (choice.statType == Weapon::UpgradeStatType::ShotCooldown) {
					const float reductionRate = -choice.calculatedAmount / choice.currentValue * 100.0f;
					ImGui::Text("現在値 → 強化後: %.3f秒 →", choice.currentValue);
					ImGui::SameLine();
					ImGui::TextColored(
						ImVec4(0.30f, 0.95f, 0.40f, 1.0f),
						"%.3f秒（%.1f%%短縮）",
						choice.currentValue + choice.calculatedAmount,
						reductionRate
					);
				} else {
					ImGui::Text("現在値 → 強化後: %.1f →", choice.currentValue);
					ImGui::SameLine();
					ImGui::TextColored(
						ImVec4(0.30f, 0.95f, 0.40f, 1.0f),
						"%.1f",
						choice.currentValue + choice.calculatedAmount
					);
				}
			} else {
				ImGui::TextDisabled("強化ステータス・レアリティなし");
			}

			ImGui::BeginDisabled(!decisionInputDelayTimer_.IsFinished() || isDecisionAnimationPlaying_);
			if (ImGui::Button("この候補を選択")) {
				selectedChoiceIndex_ = choiceIndex;
				UpdateCards(0.0f);
				BeginDecisionAnimation(choiceIndex, choice.generation);
			}
			ImGui::EndDisabled();

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
		SetOverlayVisible(true);

		const std::uint64_t currentGeneration = choices.front().generation;
		if (selectedGeneration_ != currentGeneration) {

			// 新世代の候補では左右へ一操作で移れる中央から選べるようCard内容と選択位置を再構築
			ResetDecisionAnimationState();
			decisionInputDelayTimer_.Start(kDecisionInputDelayDuration);
			MyInput::ResetStickHorizontalInput(kUpgradeStickInputName);
			selectedGeneration_ = currentGeneration;
			visibleChoiceCount_ = (std::min)(choices.size(), upgradeCards_.size());
			selectedChoiceIndex_ = visibleChoiceCount_ / 2;

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
			selectedChoiceIndex_ = visibleChoiceCount_ / 2;
		}
	}

	void UpgradeUI::UpdateCards(float deltaTime) {
		for (std::size_t cardIndex = 0; cardIndex < upgradeCards_.size(); ++cardIndex) {
			WeaponUpgradeCardUI& card = upgradeCards_[cardIndex];
			card.SetSelected(cardIndex < visibleChoiceCount_ && cardIndex == selectedChoiceIndex_);
			card.Update(deltaTime);
		}
	}

	void UpgradeUI::BeginDecisionAnimation(std::size_t choiceIndex, std::uint64_t generation) {

		// RuntimeとDebugの両経路で表示直後の決定演出開始を抑止
		if (!decisionInputDelayTimer_.IsFinished() || isDecisionAnimationPlaying_ || choiceIndex >= visibleChoiceCount_ ||
			choiceIndex >= upgradeCards_.size()) {
			return;
		}

		// 演出中の候補変更に備えてIndexと世代番号を対で保持
		selectedChoiceIndex_ = choiceIndex;
		UpdateCards(0.0f);
		decisionChoiceIndex_ = choiceIndex;
		decisionGeneration_ = generation;
		isDecisionAnimationPlaying_ = true;
		upgradeCards_[choiceIndex].PlayDecisionAnimation();
	}

	void UpgradeUI::ResetDecisionAnimationState() {
		if (isDecisionAnimationPlaying_ && decisionChoiceIndex_ < upgradeCards_.size()) {
			upgradeCards_[decisionChoiceIndex_].ResetDecisionAnimation();
		}
		decisionChoiceIndex_ = 0;
		decisionGeneration_ = 0;
		isDecisionAnimationPlaying_ = false;
	}

	void UpgradeUI::ResetSelection() {
		ResetDecisionAnimationState();
		decisionInputDelayTimer_.Reset();
		MyInput::ResetStickHorizontalInput(kUpgradeStickInputName);
		SetOverlayVisible(false);
		selectedChoiceIndex_ = 0;
		visibleChoiceCount_ = 0;
		selectedGeneration_ = 0;
		for (WeaponUpgradeCardUI& card : upgradeCards_) {
			card.SetSelected(false);
			card.SetVisible(false);
		}
	}

	void UpgradeUI::SetOverlayVisible(bool isVisible) {
		if (Sprite* overlay = MySprite::TryGet(overlaySprite_)) {
			overlay->SetVisible(isVisible);
		}
	}
}
