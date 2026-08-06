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
constexpr Vector4 kNewWeaponColor = { 0.95f, 0.97f, 1.0f, 1.0f };
constexpr const char* kCardObjectNamePrefix = "WeaponUpgradeCard";

/// @brief 配列形式の色をVector4へ変換する
/// @param color RGBA色
/// @return Vector4形式のRGBA色
Vector4 ToVector4(const std::array<float, 4>& color) {
	return { color[0], color[1], color[2], color[3] };
}

/// @brief SpriteHandleを一時参照へ解決する
/// @param handle SpriteHandle
/// @return 有効な場合はSprite、無効な場合はnullptr
Sprite* ResolveSprite(MadoEngine::SpriteHandle handle) {
	return MySprite::TryGet(handle);
}

/// @brief TextHandleを一時参照へ解決する
/// @param handle TextHandle
/// @return 有効な場合はText、無効な場合はnullptr
MadoEngine::Text* ResolveText(MadoEngine::TextHandle handle) {
	return MyText::TryGet(handle);
}

} // namespace

namespace UI::Game {

void WeaponUpgradeCardUI::Initialize(std::size_t cardIndex) {
	cardIndex_ = cardIndex;
	const std::string suffix = std::to_string(cardIndex_);
	const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;

	cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)] = MySprite::Create(
		std::string(kCardObjectNamePrefix) + "Border" + suffix,
		"white16x16",
		SceneType::Game,
		layer);
	cardSprites_[static_cast<std::size_t>(CardSpriteType::Background)] = MySprite::Create(
		std::string(kCardObjectNamePrefix) + "Background" + suffix,
		"white16x16",
		SceneType::Game,
		layer);
	cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBorder)] = MySprite::Create(
		std::string(kCardObjectNamePrefix) + "IconBorder" + suffix,
		"white16x16",
		SceneType::Game,
		layer);
	cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBackground)] = MySprite::Create(
		std::string(kCardObjectNamePrefix) + "IconBackground" + suffix,
		"white16x16",
		SceneType::Game,
		layer);
	cardIconSprite_ = MySprite::Create(
		std::string(kCardObjectNamePrefix) + "Icon" + suffix,
		"None",
		SceneType::Game,
		layer);
	weaponNameText_ = MyText::Create(
		std::string(kCardObjectNamePrefix) + "WeaponName" + suffix,
		"",
		SceneType::Game,
		layer);
	categoryText_ = MyText::Create(
		std::string(kCardObjectNamePrefix) + "Category" + suffix,
		"",
		SceneType::Game,
		layer);
	detailText_ = MyText::Create(
		std::string(kCardObjectNamePrefix) + "Detail" + suffix,
		"",
		SceneType::Game,
		layer);
	selectionText_ = MyText::Create(
		std::string(kCardObjectNamePrefix) + "Selection" + suffix,
		"選択中",
		SceneType::Game,
		layer);

	ApplyLayout();
	isInitialized_ = true;
	SetVisible(false);
}

void WeaponUpgradeCardUI::Finalize() {
	if (!isInitialized_) {
		return;
	}
	cardSprites_.fill({});
	cardIconSprite_ = {};
	weaponNameText_ = {};
	categoryText_ = {};
	detailText_ = {};
	selectionText_ = {};
	isSelected_ = false;
	isVisible_ = false;
	isInitialized_ = false;
}

void WeaponUpgradeCardUI::SetChoice(const Weapon::UpgradeChoice& choice) {
	if (Sprite* cardIconSprite = ResolveSprite(cardIconSprite_)) {
		const std::string textureName = Projectile::ProjectileTypeToString(choice.weaponType);
		if (!cardIconSprite->SetTexture(textureName)) {
			(void)cardIconSprite->SetTexture("None");
		}
	}
	if (MadoEngine::Text* weaponNameText = ResolveText(weaponNameText_)) {
		weaponNameText->SetText(choice.weaponDisplayName);
	}

	const bool isOwnedWeaponUpgrade = choice.choiceType == Weapon::UpgradeChoiceType::OwnedWeaponUpgrade;
	if (MadoEngine::Text* categoryText = ResolveText(categoryText_)) {
		if (isOwnedWeaponUpgrade) {
			categoryText->SetText(choice.rarityDisplayName + "\n" + choice.choiceTypeDisplayName);
			categoryText->SetColor(ToVector4(choice.rarityDisplayColor));
		} else {
			categoryText->SetText("NEW\n新規武器");
			categoryText->SetColor(kNewWeaponColor);
		}
	}
	if (MadoEngine::Text* detailText = ResolveText(detailText_)) {
		if (isOwnedWeaponUpgrade) {
			detailText->SetText(std::format("{}\n{:+.3f}", choice.statDisplayName, choice.calculatedAmount));
		} else {
			detailText->SetText("新しい武器を装備");
		}
	}
	if (Sprite* iconBorder = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBorder)])) {
		iconBorder->SetColor(isOwnedWeaponUpgrade ? ToVector4(choice.rarityDisplayColor) : kNewWeaponColor);
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
	Sprite* border = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)]);
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
	const float cardPositionX = kCardBasePositionX + kCardPositionDifferenceX * static_cast<float>(cardIndex_);
	const Vector2 cardPosition = { cardPositionX, kCardPositionY };
	const Vector2 iconPosition = { cardPositionX - 70.0f, 225.0f };

	for (MadoEngine::SpriteHandle handle : cardSprites_) {
		if (Sprite* sprite = ResolveSprite(handle)) {
			sprite->SetAnchorPoint({ 0.5f, 0.5f });
		}
	}
	if (Sprite* border = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)])) {
		border->SetPosition(cardPosition);
		border->SetScale(kCardBorderScale);
		border->SetColor({ 0.35f, 0.38f, 0.45f, 1.0f });
	}
	if (Sprite* background = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Background)])) {
		background->SetPosition(cardPosition);
		background->SetScale(kCardBackgroundScale);
		background->SetColor({ 0.055f, 0.07f, 0.11f, 0.96f });
	}
	if (Sprite* iconBorder = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBorder)])) {
		iconBorder->SetPosition(iconPosition);
		iconBorder->SetScale(kIconBorderScale);
	}
	if (Sprite* iconBackground = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBackground)])) {
		iconBackground->SetPosition(iconPosition);
		iconBackground->SetScale(kIconBackgroundScale);
		iconBackground->SetColor({ 0.025f, 0.03f, 0.05f, 1.0f });
	}
	if (Sprite* cardIconSprite = ResolveSprite(cardIconSprite_)) {
		cardIconSprite->SetAnchorPoint({ 0.5f, 0.5f });
		cardIconSprite->SetPosition(iconPosition);
		cardIconSprite->SetScale(kIconScale);
	}
	if (MadoEngine::Text* weaponNameText = ResolveText(weaponNameText_)) {
		weaponNameText->SetFontFamily("Yu Gothic UI");
		weaponNameText->SetFontSize(24.0f);
		weaponNameText->SetAreaSize({ 125.0f, 60.0f });
		weaponNameText->SetAnchorPoint({ 0.5f, 0.5f });
		weaponNameText->SetPosition({ cardPositionX + 38.0f, 205.0f });
		weaponNameText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		weaponNameText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		weaponNameText->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	if (MadoEngine::Text* categoryText = ResolveText(categoryText_)) {
		categoryText->SetFontFamily("Yu Gothic UI");
		categoryText->SetFontSize(17.0f);
		categoryText->SetAreaSize({ 125.0f, 70.0f });
		categoryText->SetAnchorPoint({ 0.5f, 0.5f });
		categoryText->SetPosition({ cardPositionX + 38.0f, 240.0f });
		categoryText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		categoryText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
	}
	if (MadoEngine::Text* detailText = ResolveText(detailText_)) {
		detailText->SetFontFamily("Yu Gothic UI");
		detailText->SetFontSize(22.0f);
		detailText->SetAreaSize({ 210.0f, 125.0f });
		detailText->SetAnchorPoint({ 0.5f, 0.5f });
		detailText->SetPosition({ cardPositionX, 390.0f });
		detailText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		detailText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		detailText->SetColor({ 0.92f, 0.94f, 1.0f, 1.0f });
	}
	if (MadoEngine::Text* selectionText = ResolveText(selectionText_)) {
		selectionText->SetFontFamily("Yu Gothic UI");
		selectionText->SetFontSize(20.0f);
		selectionText->SetAreaSize({ 150.0f, 36.0f });
		selectionText->SetAnchorPoint({ 0.5f, 0.5f });
		selectionText->SetPosition({ cardPositionX, 515.0f });
		selectionText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		selectionText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		selectionText->SetColor({ 1.0f, 0.82f, 0.18f, 1.0f });
	}
}

void WeaponUpgradeCardUI::ApplyVisibility() {
	for (MadoEngine::SpriteHandle handle : cardSprites_) {
		if (Sprite* sprite = ResolveSprite(handle)) {
			sprite->SetVisible(isVisible_);
		}
	}
	if (Sprite* cardIconSprite = ResolveSprite(cardIconSprite_)) {
		cardIconSprite->SetVisible(isVisible_);
	}
	if (MadoEngine::Text* weaponNameText = ResolveText(weaponNameText_)) {
		weaponNameText->SetVisible(isVisible_);
	}
	if (MadoEngine::Text* categoryText = ResolveText(categoryText_)) {
		categoryText->SetVisible(isVisible_);
	}
	if (MadoEngine::Text* detailText = ResolveText(detailText_)) {
		detailText->SetVisible(isVisible_);
	}
	if (MadoEngine::Text* selectionText = ResolveText(selectionText_)) {
		selectionText->SetVisible(isVisible_ && isSelected_);
	}
}

} // namespace UI::Game
