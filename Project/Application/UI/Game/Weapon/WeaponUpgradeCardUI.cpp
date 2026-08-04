#include "WeaponUpgradeCardUI.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace {
	constexpr float kCardBasePositionX = 360.0f;
	constexpr float kCardPositionDifferenceX = 280.0f;
	constexpr float kCardPositionY = 360.0f;
	constexpr Vector2 kCardBorderScale = { 16.0f, 24.0f };
	constexpr Vector2 kCardBackgroundScale = { 15.0f, 23.0f };
	constexpr Vector2 kIconBorderScale = { 4.8f, 4.8f };
	constexpr Vector2 kIconBackgroundScale = { 4.0f, 4.0f };
	constexpr Vector2 kIconScale = { 0.9f, 0.9f };
	constexpr const char* kCardObjectNamePrefix = "WeaponUpgradeCard";

	/// @brief std::array形式の色をVector4へ変換
	/// @param color 変換元のRGBA色
	/// @return Vector4形式のRGBA色
	Vector4 ToVector4(const std::array<float, 4>& color) {
		return { color[0], color[1], color[2], color[3] };
	}
}

namespace UI::Game {

	void WeaponUpgradeCardUI::Initialize(std::size_t cardIndex) {
		cardIndex_ = cardIndex;
		const std::string suffix = std::to_string(cardIndex_);
		const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;

		cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)] = MySprite::Create(
			std::string(kCardObjectNamePrefix) + "Border" + suffix,
			"white16x16",
			SceneType::Game,
			layer
		);
		cardSprites_[static_cast<std::size_t>(CardSpriteType::Background)] = MySprite::Create(
			std::string(kCardObjectNamePrefix) + "Background" + suffix,
			"white16x16",
			SceneType::Game,
			layer
		);
		cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBorder)] = MySprite::Create(
			std::string(kCardObjectNamePrefix) + "IconBorder" + suffix,
			"white16x16",
			SceneType::Game,
			layer
		);
		cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBackground)] = MySprite::Create(
			std::string(kCardObjectNamePrefix) + "IconBackground" + suffix,
			"white16x16",
			SceneType::Game,
			layer
		);

		cardIconSprite_ = MySprite::Create(
			std::string(kCardObjectNamePrefix) + "Icon" + suffix,
			"None",
			SceneType::Game,
			layer
		);

		weaponNameText_ = MyText::Create(
			std::string(kCardObjectNamePrefix) + "WeaponName" + suffix,
			"",
			SceneType::Game,
			layer
		);
		categoryText_ = MyText::Create(
			std::string(kCardObjectNamePrefix) + "Category" + suffix,
			"",
			SceneType::Game,
			layer
		);
		detailText_ = MyText::Create(
			std::string(kCardObjectNamePrefix) + "Detail" + suffix,
			"",
			SceneType::Game,
			layer
		);
		selectionText_ = MyText::Create(
			std::string(kCardObjectNamePrefix) + "Selection" + suffix,
			"選択中",
			SceneType::Game,
			layer
		);

		ApplyLayout();
		isInitialized_ = true;
		SetVisible(false);
	}

	void WeaponUpgradeCardUI::Finalize() {
		if (!isInitialized_) {
			return;
		}

		const std::string suffix = std::to_string(cardIndex_);
		MySprite::Destroy(std::string(kCardObjectNamePrefix) + "Border" + suffix);
		MySprite::Destroy(std::string(kCardObjectNamePrefix) + "Background" + suffix);
		MySprite::Destroy(std::string(kCardObjectNamePrefix) + "IconBorder" + suffix);
		MySprite::Destroy(std::string(kCardObjectNamePrefix) + "IconBackground" + suffix);
		MySprite::Destroy(std::string(kCardObjectNamePrefix) + "Icon" + suffix);
		MyText::Destroy(std::string(kCardObjectNamePrefix) + "WeaponName" + suffix);
		MyText::Destroy(std::string(kCardObjectNamePrefix) + "Category" + suffix);
		MyText::Destroy(std::string(kCardObjectNamePrefix) + "Detail" + suffix);
		MyText::Destroy(std::string(kCardObjectNamePrefix) + "Selection" + suffix);

		cardSprites_.fill(nullptr);
		cardIconSprite_ = nullptr;
		weaponNameText_ = nullptr;
		categoryText_ = nullptr;
		detailText_ = nullptr;
		selectionText_ = nullptr;
		isSelected_ = false;
		isVisible_ = false;
		isInitialized_ = false;
	}

	void WeaponUpgradeCardUI::SetChoice(const Weapon::UpgradeChoice& choice) {
		if (cardIconSprite_) {
			const std::string textureName = Projectile::ProjectileTypeToString(choice.weaponType);
			if (!cardIconSprite_->SetTexture(textureName)) {
				(void)cardIconSprite_->SetTexture("None");
			}
		}

		if (weaponNameText_) {
			weaponNameText_->SetText(choice.weaponDisplayName);
		}

		const bool isOwnedWeaponUpgrade =
			choice.choiceType == Weapon::UpgradeChoiceType::OwnedWeaponUpgrade;
		if (categoryText_) {
			if (isOwnedWeaponUpgrade) {
				categoryText_->SetText(choice.rarityDisplayName + "\n" + choice.choiceTypeDisplayName);
				categoryText_->SetColor(ToVector4(choice.rarityDisplayColor));
			} else {
				categoryText_->SetText("NEW\n新規武器");
				categoryText_->SetColor({ 0.25f, 0.9f, 1.0f, 1.0f });
			}
		}

		if (detailText_) {
			if (isOwnedWeaponUpgrade) {
				detailText_->SetText(std::format(
					"{}\n{:+.3f}",
					choice.statDisplayName,
					choice.calculatedAmount
				));
			} else {
				detailText_->SetText("新しい武器を装備");
			}
		}

		Sprite* iconBorder = cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBorder)];
		if (iconBorder) {
			iconBorder->SetColor(isOwnedWeaponUpgrade
				? ToVector4(choice.rarityDisplayColor)
				: Vector4{ 0.25f, 0.9f, 1.0f, 1.0f });
		}

		SetVisible(true);
	}

	void WeaponUpgradeCardUI::SetSelected(bool isSelected) {
		if (isSelected_ != isSelected) {
			selectedAnimationTime_ = 0.0f;
		}
		isSelected_ = isSelected;
		ApplyVisibility();
	}

	void WeaponUpgradeCardUI::SetVisible(bool isVisible) {
		isVisible_ = isVisible;
		if (!isVisible_) {
			selectedAnimationTime_ = 0.0f;
		}
		ApplyVisibility();
	}

	void WeaponUpgradeCardUI::Update(float deltaTime) {
		if (!isVisible_) {
			return;
		}

		Sprite* border = cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)];
		if (!border) {
			return;
		}

		if (!isSelected_) {
			selectedAnimationTime_ = 0.0f;
			border->SetScale(kCardBorderScale);
			border->SetColor({ 0.35f, 0.38f, 0.45f, 1.0f });
			return;
		}

		selectedAnimationTime_ += (std::max)(deltaTime, 0.0f);
		const float pulse = 1.0f + std::sin(selectedAnimationTime_ * 5.0f) * 0.015f;
		border->SetScale({ kCardBorderScale.x * pulse, kCardBorderScale.y * pulse });
		border->SetColor({ 1.0f, 0.82f, 0.18f, 1.0f });
	}

	void WeaponUpgradeCardUI::ApplyLayout() {
		const float cardPositionX =
			kCardBasePositionX + kCardPositionDifferenceX * static_cast<float>(cardIndex_);
		const Vector2 cardPosition = { cardPositionX, kCardPositionY };
		const Vector2 iconPosition = { cardPositionX - 70.0f, 225.0f };

		for (Sprite* sprite : cardSprites_) {
			if (sprite) {
				sprite->SetAnchorPoint({ 0.5f, 0.5f });
			}
		}

		Sprite* border = cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)];
		if (border) {
			border->SetPosition(cardPosition);
			border->SetScale(kCardBorderScale);
			border->SetColor({ 0.35f, 0.38f, 0.45f, 1.0f });
		}

		Sprite* background = cardSprites_[static_cast<std::size_t>(CardSpriteType::Background)];
		if (background) {
			background->SetPosition(cardPosition);
			background->SetScale(kCardBackgroundScale);
			background->SetColor({ 0.055f, 0.07f, 0.11f, 0.96f });
		}

		Sprite* iconBorder = cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBorder)];
		if (iconBorder) {
			iconBorder->SetPosition(iconPosition);
			iconBorder->SetScale(kIconBorderScale);
		}

		Sprite* iconBackground = cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBackground)];
		if (iconBackground) {
			iconBackground->SetPosition(iconPosition);
			iconBackground->SetScale(kIconBackgroundScale);
			iconBackground->SetColor({ 0.025f, 0.03f, 0.05f, 1.0f });
		}

		if (cardIconSprite_) {
			cardIconSprite_->SetAnchorPoint({ 0.5f, 0.5f });
			cardIconSprite_->SetPosition(iconPosition);
			cardIconSprite_->SetScale(kIconScale);
		}

		if (weaponNameText_) {
			weaponNameText_->SetFontFamily("Yu Gothic UI");
			weaponNameText_->SetFontSize(24.0f);
			weaponNameText_->SetAreaSize({ 125.0f, 60.0f });
			weaponNameText_->SetAnchorPoint({ 0.5f, 0.5f });
			weaponNameText_->SetPosition({ cardPositionX + 38.0f, 205.0f });
			weaponNameText_->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
			weaponNameText_->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
			weaponNameText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		if (categoryText_) {
			categoryText_->SetFontFamily("Yu Gothic UI");
			categoryText_->SetFontSize(17.0f);
			categoryText_->SetAreaSize({ 125.0f, 70.0f });
			categoryText_->SetAnchorPoint({ 0.5f, 0.5f });
			categoryText_->SetPosition({ cardPositionX + 38.0f, 240.0f });
			categoryText_->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
			categoryText_->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		}

		if (detailText_) {
			detailText_->SetFontFamily("Yu Gothic UI");
			detailText_->SetFontSize(22.0f);
			detailText_->SetAreaSize({ 210.0f, 125.0f });
			detailText_->SetAnchorPoint({ 0.5f, 0.5f });
			detailText_->SetPosition({ cardPositionX, 390.0f });
			detailText_->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
			detailText_->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
			detailText_->SetColor({ 0.92f, 0.94f, 1.0f, 1.0f });
		}

		if (selectionText_) {
			selectionText_->SetFontFamily("Yu Gothic UI");
			selectionText_->SetFontSize(20.0f);
			selectionText_->SetAreaSize({ 150.0f, 36.0f });
			selectionText_->SetAnchorPoint({ 0.5f, 0.5f });
			selectionText_->SetPosition({ cardPositionX, 515.0f });
			selectionText_->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
			selectionText_->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
			selectionText_->SetColor({ 1.0f, 0.82f, 0.18f, 1.0f });
		}
	}

	void WeaponUpgradeCardUI::ApplyVisibility() {
		for (Sprite* sprite : cardSprites_) {
			if (sprite) {
				sprite->SetVisible(isVisible_);
			}
		}

		if (cardIconSprite_) {
			cardIconSprite_->SetVisible(isVisible_);
		}
		if (weaponNameText_) {
			weaponNameText_->SetVisible(isVisible_);
		}
		if (categoryText_) {
			categoryText_->SetVisible(isVisible_);
		}
		if (detailText_) {
			detailText_->SetVisible(isVisible_);
		}
		if (selectionText_) {
			selectionText_->SetVisible(isVisible_ && isSelected_);
		}
	}
}
