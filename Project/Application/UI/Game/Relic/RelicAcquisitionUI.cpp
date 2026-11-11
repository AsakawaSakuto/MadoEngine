#include "RelicAcquisitionUI.h"
#include "Audio/MyAudio.h"
#include "GameObject/Relic/RelicSystem.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI
#include <algorithm>
#include <array>

namespace {

constexpr float kDecisionDelayDuration = 0.2f;
constexpr const char* kDecisionSoundKey = "RelicDecision";
constexpr const char* kRelicGroupName = "Relic";
constexpr const char* kCardViewGroupName = "CardView";
constexpr std::array<const char*, 3> kCardPanelNames = { "Overlay", "Border", "Panel" };
constexpr const char* kCardIconName = "Icon";
constexpr std::array<const char*, 3> kCardTextNames = { "Name", "Rarity", "Description" };
constexpr const char* kDecisionGroupName = "Decision";

/// @brief SpriteHandleを一時参照へ解決
/// @param handle SpriteHandle
/// @return 有効な場合はSprite、無効な場合はnullptr
Sprite* ResolveSprite(MadoEngine::SpriteHandle handle) {
	return MySprite::TryGet(handle);
}

/// @brief TextHandleを一時参照へ解決
/// @param handle TextHandle
/// @return 有効な場合はText、無効な場合はnullptr
MadoEngine::Text* ResolveText(MadoEngine::TextHandle handle) {
	return MyText::TryGet(handle);
}

}

namespace UI::Game {

	void RelicAcquisitionUI::Initialize() {
		isDisplayEnabled_ = true;
		isBackgroundEnabled_ = true;
		isInitialized_ = true;
		SetVisible(false);
	}

	void RelicAcquisitionUI::Finalize() {
		SetVisible(false);
		panelSprites_.fill({});
		textHandles_.fill({});
		iconSprite_ = {};
		displayedRelicId_.clear();
		displayedRevision_ = 0;
		isVisible_ = false;
		isInitialized_ = false;
		isDisplayEnabled_ = true;
		isBackgroundEnabled_ = true;
	}

	void RelicAcquisitionUI::SetDisplayEnabled(bool isDisplayEnabled) {
		isDisplayEnabled_ = isDisplayEnabled;

		// Chest表示開始Frameの描画より前に既存のカードやプレビューを非表示化
		if (!isDisplayEnabled_) {
			SetVisible(false);
		}
	}

	void RelicAcquisitionUI::SetBackgroundEnabled(bool isBackgroundEnabled) {
		isBackgroundEnabled_ = isBackgroundEnabled;
		RefreshCardBindings();
		if (Sprite* overlay = ResolveSprite(panelSprites_[0])) {
			overlay->SetRuntimeVisible(isBackgroundEnabled_);
		}
	}

	void RelicAcquisitionUI::Update(float deltaTime, Relic::System& relicSystem, bool acceptDecisionInput) {

		// 開封待ち中は非表示のカードに対する獲得確定入力も禁止
		if (!isDisplayEnabled_) {
			SetVisible(false);
			return;
		}

		const Relic::Definition* definition = relicSystem.GetPendingReward();
		if (!definition) {
			SetVisible(false);
			return;
		}

		if (!isVisible_ || displayedRelicId_ != definition->id ||
			displayedRevision_ != relicSystem.GetRevision()) {
			ResetDecisionDelay();
			displayedRevision_ = relicSystem.GetRevision();
			SetVisible(true);
		}

		// 同じ報酬でもEditor再読込後の要素へ内容を同期し、配置やフォントの編集値を維持
		SetDefinition(*definition);
		UpdateDecisionControl(deltaTime);

		// 保留報酬を維持してWaitingGetItemを継続し、押下Frameの時間を含めず決定後の演出時間を確保
		if (decisionDelayTimer_.IsActive() || decisionDelayTimer_.IsFinished()) {

			// 待機中にEditorで報酬や所持品が変更された場合は古い決定の自動適用を中止
			if (decisionRelicId_ != definition->id || decisionRevision_ != relicSystem.GetRevision()) {
				ResetDecisionDelay();
				return;
			}
			decisionDelayTimer_.Update(deltaTime);
			if (!decisionDelayTimer_.IsFinished()) {
				return;
			}
			ResetDecisionDelay();
			if (relicSystem.ConfirmPendingReward()) {
				SetVisible(false);
				displayedRelicId_.clear();
				displayedRevision_ = 0;
			}
			return;
		}

		// 前段の開封入力で獲得表示まで閉じないよう表示開始と決定受付を分離
		if (acceptDecisionInput && MyInput::Trigger("Decision")) {
			decisionRelicId_ = definition->id;
			decisionRevision_ = relicSystem.GetRevision();
			decisionDelayTimer_.Start(kDecisionDelayDuration);
			MyAudio::Play(kDecisionSoundKey);
		}
	}

	void RelicAcquisitionUI::DrawImGui(Relic::System& relicSystem) {
#ifdef USE_IMGUI
		if (!isInitialized_) {
			return;
		}
		ImGui::Begin("レリック獲得UI");
		ImGui::Checkbox("プレビュー表示", &isPreviewEnabled_);
		const std::vector<Relic::Definition>& definitions = relicSystem.GetDefinitions();
		if (!definitions.empty()) {
			previewRelicIndex_ = (std::min)(previewRelicIndex_, definitions.size() - 1);
			ImGui::SetNextItemWidth(260.0f);
			if (ImGui::BeginCombo("プレビューするレリック", definitions[previewRelicIndex_].displayName.c_str())) {
				for (std::size_t index = 0; index < definitions.size(); ++index) {
					const bool isSelected = index == previewRelicIndex_;
					const std::string label = definitions[index].displayName + "##RelicAcquisitionPreview" + definitions[index].id;
					if (ImGui::Selectable(label.c_str(), isSelected)) {
						previewRelicIndex_ = index;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
		ImGui::End();

		// 開封許可と保留報酬を優先し、獲得中以外は選択したプレビューだけを表示
		const Relic::Definition* displayDefinition = nullptr;
		if (isDisplayEnabled_) {
			displayDefinition = relicSystem.GetPendingReward();
			if (!displayDefinition && isPreviewEnabled_ && !definitions.empty()) {
				displayDefinition = &definitions[previewRelicIndex_];
			}
		}
		if (displayDefinition) {
			SetDefinition(*displayDefinition);
			displayedRevision_ = relicSystem.GetRevision();
		}
		SetVisible(displayDefinition != nullptr);
#else
		(void)relicSystem;
#endif // USE_IMGUI
	}

	void RelicAcquisitionUI::RefreshCardBindings() {
		panelSprites_.fill({});
		textHandles_.fill({});
		iconSprite_ = {};
		auto& groups = MadoEngine::UIGroupManager::GetInstance();
		const auto relic = groups.Find(kRelicGroupName, SceneType::Game);
		const auto cardView = groups.FindChildGroup(relic, kCardViewGroupName);
		for (const auto& element : groups.GetElements(cardView)) {
			if (!element.resolved) {
				continue;
			}
			if (element.type == MadoEngine::UIGroupElementType::Sprite) {
				if (element.name == kCardIconName) {
					iconSprite_ = element.sprite;
				}
				for (std::size_t index = 0; index < kCardPanelNames.size(); ++index) {
					if (element.name == kCardPanelNames[index]) {
						panelSprites_[index] = element.sprite;
						break;
					}
				}
			} else if (element.type == MadoEngine::UIGroupElementType::Text) {
				for (std::size_t index = 0; index < kCardTextNames.size(); ++index) {
					if (element.name == kCardTextNames[index]) {
						textHandles_[index] = element.text;
						break;
					}
				}
			}
		}
	}

	void RelicAcquisitionUI::SetDefinition(const Relic::Definition& definition) {
		RefreshCardBindings();
		displayedRelicId_ = definition.id;
		const auto& color = GetRarityDefaultColor(definition.rarity);
		const Vector4 rarityColor{ color[0], color[1], color[2], color[3] };
		if (Sprite* border = ResolveSprite(panelSprites_[1])) {
			border->SetColor(rarityColor);
		}
		if (Sprite* icon = ResolveSprite(iconSprite_)) {
			if (icon->GetTextureName() != definition.textureName && !icon->SetTexture(definition.textureName)) {
				(void)icon->SetTexture("None");
			}
		}
		if (MadoEngine::Text* name = ResolveText(textHandles_[0])) {
			name->SetText(definition.displayName);
		}
		if (MadoEngine::Text* rarity = ResolveText(textHandles_[1])) {
			rarity->SetText(GetRarityDisplayName(definition.rarity));
			rarity->SetColor(rarityColor);
		}
		if (MadoEngine::Text* description = ResolveText(textHandles_[2])) {
			description->SetText(definition.description);
		}
	}

	void RelicAcquisitionUI::SetVisible(bool isVisible) {

		// 開封待ち中の描画を抑止し、親Groupの表示切替でカードと決定案内を一括管理
		isVisible = isVisible && isDisplayEnabled_;
		if (!isVisible) {
			ResetDecisionDelay();
		}
		isVisible_ = isVisible;
		UpdateDecisionControl(0.0f);
	}

	void RelicAcquisitionUI::UpdateDecisionControl(float deltaTime) {
		RefreshCardBindings();
		auto& groupManager = MadoEngine::UIGroupManager::GetInstance();
		const auto relicHandle = groupManager.Find(kRelicGroupName, SceneType::Game);
		MadoEngine::UIGroup* relicGroup = groupManager.TryGet(relicHandle);
		if (!relicGroup) {
			decisionControlFeedback_.Reset();
			return;
		}

		// 親の表示だけを切り替え、決定ボタンの個別設定と保存用の表示状態を維持
		relicGroup->SetVisible(isVisible_ && relicGroup->GetAuthoringState().visible);
		if (Sprite* overlay = ResolveSprite(panelSprites_[0])) {
			overlay->SetRuntimeVisible(isBackgroundEnabled_);
		}

		// 保存済みの描画要素もHUDより手前へ並べ、暗幕・枠・パネル・アイコンの順序を保証
		auto& sprites = MadoEngine::SpriteManager::GetInstance();
		if (relicGroup->IsVisibleInHierarchy()) {
			for (const auto handle : panelSprites_) {
				sprites.MoveToFront(handle);
			}
			sprites.MoveToFront(iconSprite_);
		}
		const auto decisionHandle = groupManager.FindChildGroup(relicHandle, kDecisionGroupName);

		// カードの前面化後に決定ボタンを更新し、ボタンをカードより手前に配置
		decisionControlFeedback_.Update(deltaTime, kDecisionGroupName, decisionHandle);
	}

	void RelicAcquisitionUI::ResetDecisionDelay() {
		decisionDelayTimer_.Reset();
		decisionRelicId_.clear();
		decisionRevision_ = 0;
	}

}
