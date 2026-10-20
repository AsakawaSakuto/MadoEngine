#include "RelicInventoryUI.h"
#include "GameObject/Relic/RelicSystem.h"
#include <format>
#include <limits>

namespace {

constexpr float kScreenCenterX = 640.0f;
constexpr float kSlotCenterY = 670.0f;
constexpr float kSlotSpacing = 64.0f;
constexpr Vector2 kIconScale = { 0.75f, 0.75f };
constexpr Vector2 kCountOffset = { 20.0f, 20.0f };
constexpr Vector2 kCountAreaSize = { 30.0f, 22.0f };
constexpr float kCountFontSize = 17.0f;
constexpr Vector4 kCountTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };

}

namespace UI::Game {

	void RelicInventoryUI::Initialize(const Relic::System& relicSystem) {
		isInitialized_ = true;
		displayedRevision_ = (std::numeric_limits<std::uint64_t>::max)();
		Update(relicSystem);
	}

	void RelicInventoryUI::Finalize() {
		slots_.clear();
		activeSlotCount_ = 0;
		displayedRevision_ = 0;
		isInitialized_ = false;
	}

	void RelicInventoryUI::SetVisible(bool isVisible) {
		isVisible_ = isVisible;

		// 再利用のため保持している未使用Slotを表示へ戻さない有効数の確認
		for (std::size_t slotIndex = 0; slotIndex < slots_.size(); ++slotIndex) {
			SetSlotVisible(slotIndex, slotIndex < activeSlotCount_);
		}
	}

	void RelicInventoryUI::Update(const Relic::System& relicSystem) {
		if (!isInitialized_ || displayedRevision_ == relicSystem.GetRevision()) {
			return;
		}

		std::vector<const Relic::Definition*> ownedDefinitions;
		for (const Relic::Definition& definition : relicSystem.GetDefinitions()) {
			if (relicSystem.GetInventory().GetCount(definition.id) > 0) {
				ownedDefinitions.push_back(&definition);
			}
		}

		EnsureSlotCount(ownedDefinitions.size());
		activeSlotCount_ = ownedDefinitions.size();
		const float firstSlotX = kScreenCenterX -
			static_cast<float>([&]() -> std::size_t {
				if (activeSlotCount_ > 0) {
					return (activeSlotCount_ - 1);
				} else {
					return (0);
				}
			}()) *
			kSlotSpacing * 0.5f;

		// カタログ順を維持して獲得順による表示位置の揺れを防止
		for (std::size_t slotIndex = 0; slotIndex < activeSlotCount_; ++slotIndex) {
			const Relic::Definition& definition = *ownedDefinitions[slotIndex];
			ConfigureSlot(
				slotIndex,
				definition,
				relicSystem.GetInventory().GetCount(definition.id),
				{ firstSlotX + static_cast<float>(slotIndex) * kSlotSpacing, kSlotCenterY });
		}

		// 定義削除や破棄で余ったスロットは再利用のため保持して非表示化
		for (std::size_t slotIndex = activeSlotCount_; slotIndex < slots_.size(); ++slotIndex) {
			SetSlotVisible(slotIndex, false);
		}

		displayedRevision_ = relicSystem.GetRevision();
	}

	void RelicInventoryUI::EnsureSlotCount(std::size_t requiredCount) {
		const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;
		while (slots_.size() < requiredCount) {
			const std::size_t slotIndex = slots_.size();
			const std::string suffix = std::to_string(slotIndex);
			Slot slot;
			slot.icon = MySprite::Create(
				"RelicInventoryIcon" + suffix,
				"None",
				SceneType::Game,
				layer);
			slot.countText = MyText::Create(
				"RelicInventoryCount" + suffix,
				"",
				SceneType::Game,
				MadoEngine::EditorManagementMode::RuntimeOnly,
				layer);

			if (Sprite* icon = MySprite::TryGet(slot.icon)) {
				icon->SetAnchorPoint({ 0.5f, 0.5f });
				icon->SetScale(kIconScale);
			}
			if (MadoEngine::Text* countText = MyText::TryGet(slot.countText)) {
				countText->SetFontAsset("Assets/Font/dot.ttf", "dot");
				countText->SetFontSize(kCountFontSize);
				countText->SetAreaSize(kCountAreaSize);
				countText->SetAnchorPoint({ 0.5f, 0.5f });
				countText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
				countText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
				countText->SetWordWrap(false);
				countText->SetColor(kCountTextColor);
			}

			slots_.push_back(slot);
		}
	}

	void RelicInventoryUI::ConfigureSlot(
		std::size_t slotIndex,
		const Relic::Definition& definition,
		int stackCount,
		const Vector2& position) {
		if (slotIndex >= slots_.size()) {
			return;
		}

		Slot& slot = slots_[slotIndex];
		if (Sprite* icon = MySprite::TryGet(slot.icon)) {
			icon->SetPosition(position);
			if (!icon->SetTexture(definition.textureName)) {
				(void)icon->SetTexture("None");
			}
		}

		const Vector2 countPosition = position + kCountOffset;
		if (MadoEngine::Text* countText = MyText::TryGet(slot.countText)) {
			countText->SetPosition(countPosition);
			countText->SetText(std::format("x{}", stackCount));
		}
		SetSlotVisible(slotIndex, true);
	}

	void RelicInventoryUI::SetSlotVisible(std::size_t slotIndex, bool isVisible) {
		if (slotIndex >= slots_.size()) {
			return;
		}

		const Slot& slot = slots_[slotIndex];
		if (Sprite* icon = MySprite::TryGet(slot.icon)) {
			icon->SetVisible(isVisible_ && isVisible);
		}
		if (MadoEngine::Text* countText = MyText::TryGet(slot.countText)) {
			countText->SetVisible(isVisible);
			countText->SetRuntimeVisible(isVisible_);
		}
	}

}
