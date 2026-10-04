#include "RelicAcquisitionUI.h"
#include "GameObject/Relic/RelicSystem.h"
#include "Input/MyInput.h"
#include <array>
#include <format>

namespace {

constexpr Vector2 kScreenCenter = { 640.0f, 360.0f };

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

/// @brief レアリティに対応する表示色を取得
/// @param rarity 色を取得するレアリティ
/// @return レアリティ表示色
Vector4 GetRarityColor(Rarity rarity) {
	switch (rarity) {
	case Rarity::Uncommon:
		return { 0.32f, 0.88f, 0.40f, 1.0f };
	case Rarity::Rare:
		return { 0.30f, 0.58f, 1.0f, 1.0f };
	case Rarity::Epic:
		return { 0.72f, 0.36f, 1.0f, 1.0f };
	case Rarity::Legendary:
		return { 1.0f, 0.75f, 0.18f, 1.0f };
	default:
		return { 1.0f, 0.30f, 0.30f, 1.0f };
	}
}

}

namespace UI::Game {

	void RelicAcquisitionUI::Initialize() {
		const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;
		panelSprites_[0] = MySprite::Create("RelicAcquisitionOverlay", "white16x16", SceneType::Game, layer);
		panelSprites_[1] = MySprite::Create("RelicAcquisitionBorder", "white16x16", SceneType::Game, layer);
		panelSprites_[2] = MySprite::Create("RelicAcquisitionPanel", "white16x16", SceneType::Game, layer);
		iconSprite_ = MySprite::Create("RelicAcquisitionIcon", "None", SceneType::Game, layer);

		const std::array<const char*, 5> objectNames = {
			"RelicAcquisitionTitle",
			"RelicAcquisitionName",
			"RelicAcquisitionRarity",
			"RelicAcquisitionDescription",
			"RelicAcquisitionGuide",
		};
		for (std::size_t index = 0; index < textHandles_.size(); ++index) {
			textHandles_[index] = MyText::Create(objectNames[index], "", SceneType::Game, layer);
		}

		// 全要素の基準を画面中央へ統一して解像度内の一枚表示を構成
		if (Sprite* overlay = ResolveSprite(panelSprites_[0])) {
			overlay->SetAnchorPoint({ 0.5f, 0.5f });
			overlay->SetPosition(kScreenCenter);
			overlay->SetScale({ 80.0f, 45.0f });
			overlay->SetColor({ 0.0f, 0.0f, 0.0f, 0.72f });
		}
		if (Sprite* border = ResolveSprite(panelSprites_[1])) {
			border->SetAnchorPoint({ 0.5f, 0.5f });
			border->SetPosition(kScreenCenter);
			border->SetScale({ 27.0f, 20.5f });
		}
		if (Sprite* panel = ResolveSprite(panelSprites_[2])) {
			panel->SetAnchorPoint({ 0.5f, 0.5f });
			panel->SetPosition(kScreenCenter);
			panel->SetScale({ 26.0f, 19.5f });
			panel->SetColor({ 0.045f, 0.06f, 0.10f, 0.98f });
		}
		if (Sprite* icon = ResolveSprite(iconSprite_)) {
			icon->SetAnchorPoint({ 0.5f, 0.5f });
			icon->SetPosition({ kScreenCenter.x, 315.0f });
			icon->SetScale({ 2.0f, 2.0f });
		}

		const std::array<Vector2, 5> textPositions = {
			Vector2{ kScreenCenter.x, 205.0f },
			Vector2{ kScreenCenter.x, 415.0f },
			Vector2{ kScreenCenter.x, 452.0f },
			Vector2{ kScreenCenter.x, 495.0f },
			Vector2{ kScreenCenter.x, 550.0f },
		};
		const std::array<float, 5> fontSizes = { 30.0f, 26.0f, 20.0f, 22.0f, 18.0f };
		for (std::size_t index = 0; index < textHandles_.size(); ++index) {
			if (MadoEngine::Text* text = ResolveText(textHandles_[index])) {
				text->SetFontFamily("Yu Gothic UI");
				text->SetFontSize(fontSizes[index]);
				text->SetAreaSize({ 480.0f, 48.0f });
				text->SetAnchorPoint({ 0.5f, 0.5f });
				text->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
				text->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
				text->SetPosition(textPositions[index]);
				text->SetColor({ 0.95f, 0.97f, 1.0f, 1.0f });
			}
		}

		if (MadoEngine::Text* title = ResolveText(textHandles_[0])) {
			title->SetText("レリック獲得");
		}
		if (MadoEngine::Text* guide = ResolveText(textHandles_[4])) {
			guide->SetText("決定で取得");
			guide->SetColor({ 1.0f, 0.82f, 0.18f, 1.0f });
		}

		isInitialized_ = true;
		SetVisible(false);
	}

	void RelicAcquisitionUI::Finalize() {
		panelSprites_.fill({});
		textHandles_.fill({});
		iconSprite_ = {};
		displayedRelicId_.clear();
		displayedRevision_ = 0;
		isVisible_ = false;
		isInitialized_ = false;
	}

	void RelicAcquisitionUI::Update(Relic::System& relicSystem) {
		const Relic::Definition* definition = relicSystem.GetPendingReward();
		if (!definition) {
			SetVisible(false);
			return;
		}

		if (!isVisible_ || displayedRelicId_ != definition->id ||
			displayedRevision_ != relicSystem.GetRevision()) {
			SetDefinition(*definition, relicSystem.GetInventory().GetCount(definition->id));
			displayedRevision_ = relicSystem.GetRevision();
			SetVisible(true);
		}

		if (MyInput::Trigger("Decision") && relicSystem.ConfirmPendingReward()) {
			SetVisible(false);
			displayedRelicId_.clear();
			displayedRevision_ = 0;
		}
	}

	void RelicAcquisitionUI::SetDefinition(const Relic::Definition& definition, int ownedCount) {
		displayedRelicId_ = definition.id;
		const Vector4 rarityColor = GetRarityColor(definition.rarity);
		if (Sprite* border = ResolveSprite(panelSprites_[1])) {
			border->SetColor(rarityColor);
		}
		if (Sprite* icon = ResolveSprite(iconSprite_)) {
			if (!icon->SetTexture(definition.textureName)) {
				(void)icon->SetTexture("None");
			}
		}
		if (MadoEngine::Text* name = ResolveText(textHandles_[1])) {
			name->SetText(definition.displayName);
		}
		if (MadoEngine::Text* rarity = ResolveText(textHandles_[2])) {
			rarity->SetText(std::format(
				"{}  所持数: {} / {}",
				Relic::ToDisplayName(definition.rarity),
				ownedCount + 1,
				definition.maxStack));
			rarity->SetColor(rarityColor);
		}
		if (MadoEngine::Text* description = ResolveText(textHandles_[3])) {
			description->SetText(definition.description);
		}
	}

	void RelicAcquisitionUI::SetVisible(bool isVisible) {
		isVisible_ = isVisible;
		for (MadoEngine::SpriteHandle handle : panelSprites_) {
			if (Sprite* sprite = ResolveSprite(handle)) {
				sprite->SetVisible(isVisible);
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
	}

}
