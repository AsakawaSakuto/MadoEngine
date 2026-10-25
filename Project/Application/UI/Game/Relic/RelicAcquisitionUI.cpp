#include "RelicAcquisitionUI.h"
#include "GameObject/Relic/RelicSystem.h"
#include "Input/MyInput.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI
#include <algorithm>
#include <array>

namespace {

constexpr float kDecisionDelayDuration = 0.2f;
constexpr UI::Game::InputControlFeedbackDefinition kDecisionControlDefinition = {
	"Decision", { "RelicDecisionBottom", "RelicDecisionBack1", "RelicDecisionBack2" }, "RelicDecisionText"
};
constexpr std::array<const char*, 3> kDecisionSpriteNames = {
	"RelicDecisionBack1", "RelicDecisionBack2", "RelicDecisionBottom"
};

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

	RelicAcquisitionUI::AppearanceSettings::AppearanceSettings() {

		// 描画型に依存しない共通色からUI用Vector4への変換
		for (const RarityDefinition& definition : kRarityDefinitions) {
			const std::size_t index = static_cast<std::size_t>(definition.rarity);
			const RarityColor& color = definition.displayColor;
			rarityColors[index] = Vector4{ color[0], color[1], color[2], color[3] };
		}
	}

	void RelicAcquisitionUI::Initialize() {
		const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;
		panelSprites_[0] = MySprite::Create("RelicAcquisitionOverlay", "white16x16", SceneType::Game, layer);
		panelSprites_[1] = MySprite::Create("RelicAcquisitionBorder", "white16x16", SceneType::Game, layer);
		panelSprites_[2] = MySprite::Create("RelicAcquisitionPanel", "white16x16", SceneType::Game, layer);
		iconSprite_ = MySprite::Create("RelicAcquisitionIcon", "None", SceneType::Game, layer);

		const std::array<const char*, 3> objectNames = {
			"RelicAcquisitionName",
			"RelicAcquisitionRarity",
			"RelicAcquisitionDescription",
		};
		for (std::size_t index = 0; index < textHandles_.size(); ++index) {
			textHandles_[index] = MyText::Create(objectNames[index], "", SceneType::Game, layer);
		}

		ApplyAppearanceSettings();

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
		if (Sprite* overlay = ResolveSprite(panelSprites_[0])) {
			overlay->SetVisible(isVisible_ && isDisplayEnabled_ && isBackgroundEnabled_);
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
			SetDefinition(*definition);
			displayedRevision_ = relicSystem.GetRevision();
			SetVisible(true);
		}

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
		}
	}

	void RelicAcquisitionUI::DrawImGui(Relic::System& relicSystem) {

#ifdef USE_IMGUI

		if (!isInitialized_) {
			return;
		}

		bool settingsChanged = false;
		bool previewContentChanged = false;
		ImGui::Begin("レリック獲得UI");
		previewContentChanged |= ImGui::Checkbox("プレビュー表示", &isPreviewEnabled_);
		ImGui::SameLine();
		if (ImGui::Button("初期値へ戻す")) {
			appearanceSettings_ = {};
			settingsChanged = true;
		}

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
						previewContentChanged = true;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		if (ImGui::CollapsingHeader("背景とパネル", ImGuiTreeNodeFlags_DefaultOpen)) {
			constexpr std::array<const char*, 3> panelNames = { "オーバーレイ", "枠", "パネル" };
			for (std::size_t index = 0; index < panelNames.size(); ++index) {
				ImGui::PushID(static_cast<int>(index));
				ImGui::SeparatorText(panelNames[index]);
				settingsChanged |= ImGui::DragFloat2("位置", &appearanceSettings_.panelPositions[index].x, 1.0f, -4096.0f, 4096.0f, "%.0f px");
				settingsChanged |= ImGui::DragFloat2("スケール", &appearanceSettings_.panelScales[index].x, 0.1f, 0.01f, 200.0f, "%.2f");
				ImGui::PopID();
			}
			settingsChanged |= ImGui::ColorEdit4("オーバーレイ色", &appearanceSettings_.overlayColor.x, ImGuiColorEditFlags_AlphaBar);
			settingsChanged |= ImGui::ColorEdit4("パネル色", &appearanceSettings_.panelColor.x, ImGuiColorEditFlags_AlphaBar);
		}

		if (ImGui::CollapsingHeader("アイコン", ImGuiTreeNodeFlags_DefaultOpen)) {
			settingsChanged |= ImGui::DragFloat2("位置##RelicIcon", &appearanceSettings_.iconPosition.x, 1.0f, -4096.0f, 4096.0f, "%.0f px");
			settingsChanged |= ImGui::DragFloat2("スケール##RelicIcon", &appearanceSettings_.iconScale.x, 0.05f, 0.01f, 100.0f, "%.2f");
		}

		if (ImGui::CollapsingHeader("文字", ImGuiTreeNodeFlags_DefaultOpen)) {
			constexpr std::array<const char*, 3> textNames = { "名前", "レアリティ", "説明" };
			for (std::size_t index = 0; index < textNames.size(); ++index) {
				ImGui::PushID(static_cast<int>(index));
				if (ImGui::TreeNode(textNames[index])) {
					settingsChanged |= ImGui::DragFloat2("位置", &appearanceSettings_.textPositions[index].x, 1.0f, -4096.0f, 4096.0f, "%.0f px");
					settingsChanged |= ImGui::DragFloat2("表示領域", &appearanceSettings_.textAreaSizes[index].x, 1.0f, 1.0f, 4096.0f, "%.0f px");
					settingsChanged |= ImGui::DragFloat("フォントサイズ", &appearanceSettings_.fontSizes[index], 1.0f, 1.0f, 200.0f, "%.0f px");
					if (index != 1) {
						settingsChanged |= ImGui::ColorEdit4("文字色", &appearanceSettings_.textColors[index].x, ImGuiColorEditFlags_AlphaBar);
					} else {
						ImGui::TextDisabled("文字色はレアリティ色を使用");
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("レアリティ色", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (const RarityDefinition& definition : kRarityDefinitions) {
				const std::size_t index = static_cast<std::size_t>(definition.rarity);
				ImGui::PushID(static_cast<int>(index));
				settingsChanged |= ImGui::ColorEdit4(
					definition.displayName,
					&appearanceSettings_.rarityColors[index].x,
					ImGuiColorEditFlags_AlphaBar);
				ImGui::PopID();
			}
		}
		ImGui::End();

		// 手入力された値も描画Objectが扱える範囲へ制限して不正なテクスチャ寸法を防止
		for (Vector2& scale : appearanceSettings_.panelScales) {
			scale.x = std::clamp(scale.x, 0.01f, 200.0f);
			scale.y = std::clamp(scale.y, 0.01f, 200.0f);
		}
		appearanceSettings_.iconScale.x = std::clamp(appearanceSettings_.iconScale.x, 0.01f, 100.0f);
		appearanceSettings_.iconScale.y = std::clamp(appearanceSettings_.iconScale.y, 0.01f, 100.0f);
		for (std::size_t index = 0; index < appearanceSettings_.fontSizes.size(); ++index) {
			appearanceSettings_.textAreaSizes[index].x = std::clamp(appearanceSettings_.textAreaSizes[index].x, 1.0f, 4096.0f);
			appearanceSettings_.textAreaSizes[index].y = std::clamp(appearanceSettings_.textAreaSizes[index].y, 1.0f, 4096.0f);
			appearanceSettings_.fontSizes[index] = std::clamp(appearanceSettings_.fontSizes[index], 1.0f, 200.0f);
		}

		if (settingsChanged) {
			ApplyAppearanceSettings();
		}

		// 開封前の報酬をImGui経由で再表示しないよう通常表示と同じ許可状態を使用
		const Relic::Definition* displayDefinition = nullptr;
		if (isDisplayEnabled_) {
			displayDefinition = relicSystem.GetPendingReward();
			if (!displayDefinition && isPreviewEnabled_ && !definitions.empty()) {
				displayDefinition = &definitions[previewRelicIndex_];
			}
		}

		// 実報酬を優先し、待機中でない場合だけEditor用レリックへ表示内容を差し替え
		if (displayDefinition &&
			(settingsChanged || previewContentChanged || displayedRelicId_ != displayDefinition->id)) {
			SetDefinition(*displayDefinition);
			displayedRevision_ = relicSystem.GetRevision();
		}
		SetVisible(displayDefinition != nullptr);

#else

		(void)relicSystem;

#endif // USE_IMGUI
	}

	void RelicAcquisitionUI::ApplyAppearanceSettings() {
		for (std::size_t index = 0; index < panelSprites_.size(); ++index) {
			if (Sprite* sprite = ResolveSprite(panelSprites_[index])) {
				sprite->SetAnchorPoint({ 0.5f, 0.5f });
				sprite->SetPosition(appearanceSettings_.panelPositions[index]);
				sprite->SetScale(appearanceSettings_.panelScales[index]);
			}
		}
		if (Sprite* overlay = ResolveSprite(panelSprites_[0])) {
			overlay->SetColor(appearanceSettings_.overlayColor);
		}
		if (Sprite* panel = ResolveSprite(panelSprites_[2])) {
			panel->SetColor(appearanceSettings_.panelColor);
		}
		if (Sprite* icon = ResolveSprite(iconSprite_)) {
			icon->SetAnchorPoint({ 0.5f, 0.5f });
			icon->SetPosition(appearanceSettings_.iconPosition);
			icon->SetScale(appearanceSettings_.iconScale);
		}

		for (std::size_t index = 0; index < textHandles_.size(); ++index) {
			if (MadoEngine::Text* text = ResolveText(textHandles_[index])) {
				text->SetFontFamily("Yu Gothic UI");
				text->SetFontSize(appearanceSettings_.fontSizes[index]);
				text->SetAreaSize(appearanceSettings_.textAreaSizes[index]);
				text->SetAnchorPoint({ 0.5f, 0.5f });
				text->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
				text->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
				text->SetPosition(appearanceSettings_.textPositions[index]);
				text->SetColor(appearanceSettings_.textColors[index]);
			}
		}
	}

	const Vector4& RelicAcquisitionUI::GetRarityColor(Rarity rarity) const {
		const std::size_t index = static_cast<std::size_t>(rarity);
		if (index >= appearanceSettings_.rarityColors.size()) {
			return appearanceSettings_.rarityColors.back();
		}
		return appearanceSettings_.rarityColors[index];
	}

	void RelicAcquisitionUI::SetDefinition(const Relic::Definition& definition) {
		displayedRelicId_ = definition.id;
		const Vector4& rarityColor = GetRarityColor(definition.rarity);
		if (Sprite* border = ResolveSprite(panelSprites_[1])) {
			border->SetColor(rarityColor);
		}
		if (Sprite* icon = ResolveSprite(iconSprite_)) {
			if (!icon->SetTexture(definition.textureName)) {
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

		// 表示要求の発生元にかかわらず開封待ち中の描画を抑止
		isVisible = isVisible && isDisplayEnabled_;
		if (!isVisible) {
			ResetDecisionDelay();
		}
		isVisible_ = isVisible;

		// フラッシュ中の背景暗転だけを抑止し、ImGui経由の表示要求にも同じ条件を適用
		for (std::size_t index = 0; index < panelSprites_.size(); ++index) {
			bool isPanelVisible = isVisible;
			if (index == 0) {
				isPanelVisible = isPanelVisible && isBackgroundEnabled_;
			}
			if (Sprite* sprite = ResolveSprite(panelSprites_[index])) {
				sprite->SetVisible(isPanelVisible);
			}
		}
		if (Sprite* icon = ResolveSprite(iconSprite_)) {
			icon->SetVisible(isVisible);
		}
		for (MadoEngine::TextHandle handle : textHandles_) {
			if (MadoEngine::Text* text = ResolveText(handle)) {
				text->SetVisible(isVisible);
			}
		}
		UpdateDecisionControl(0.0f);
	}

	void RelicAcquisitionUI::UpdateDecisionControl(float deltaTime) {

		// 動的生成した暗幕の直後へ案内を配置し、Editor配置のSpriteが暗幕の背後へ隠れる状態を防止
		MadoEngine::SpriteHandle previousSprite = panelSprites_[0];
		for (const char* name : kDecisionSpriteNames) {
			const MadoEngine::SpriteHandle handle = MySprite::Find(name);
			Sprite* sprite = ResolveSprite(handle);
			if (!sprite || sprite->GetSceneType() != SceneType::Game) {
				continue;
			}
			sprite->SetVisible(isVisible_);
			if (isVisible_ && MySprite::MoveAfter(handle, previousSprite)) {
				previousSprite = handle;
			}
		}

		// Text再生成やカード非表示にも追従し、Upgradeと共通の基準Scale復元を適用
		MadoEngine::Text* text = ResolveText(MyText::Find(kDecisionControlDefinition.textName));
		if (text && text->GetSceneType() == SceneType::Game) {
			text->SetVisible(isVisible_);
		}
		decisionControlFeedback_.Update(deltaTime, isVisible_, kDecisionControlDefinition);
	}

	void RelicAcquisitionUI::ResetDecisionDelay() {
		decisionDelayTimer_.Reset();
		decisionRelicId_.clear();
		decisionRevision_ = 0;
	}

}
