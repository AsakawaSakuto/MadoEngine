#include "WeaponUpgradeCardUI.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
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
constexpr float kSelectedCardScale = 1.08f;          // 選択中カードの拡大率
constexpr float kSelectedCardPulseScale = 0.01f;     // 選択中カードの拡縮幅
constexpr float kScaleTransitionDuration = 0.18f;    // 選択状態の拡大率を適用する際の補間時間
constexpr float kSelectedCardPulseDuration = 0.25f;  // 選択中カードの拡縮周期
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

/// @brief 基準位置を中心として座標を拡大する
/// @param position 拡大する座標
/// @param center 拡大の中心座標
/// @param scale 拡大率
/// @return 拡大後の座標
Vector2 ScalePositionAroundCenter(const Vector2& position, const Vector2& center, float scale) {
	return {
		center.x + (position.x - center.x) * scale,
		center.y + (position.y - center.y) * scale,
	};
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
	scaleTransitionTimer_.Reset();
	selectedPulseTimer_.Reset();
	scaleTransitionStart_ = 1.0f;
	currentScale_ = 1.0f;
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
		scaleTransitionStart_ = currentScale_;
		scaleTransitionTimer_.Start(kScaleTransitionDuration, false);
		selectedPulseTimer_.Reset();
	}
	isSelected_ = isSelected;
	ApplyVisibility();
}

void WeaponUpgradeCardUI::SetVisible(bool isVisible) {
	isVisible_ = isVisible;
	if (!isVisible_) {
		scaleTransitionTimer_.Reset();
		selectedPulseTimer_.Reset();
		scaleTransitionStart_ = 1.0f;
		currentScale_ = 1.0f;
		ApplySelectionScale(currentScale_);
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
	const float safeDeltaTime = (std::max)(deltaTime, 0.0f);
	scaleTransitionTimer_.Update(safeDeltaTime);
	const float transitionProgress = scaleTransitionTimer_.GetProgress();
	const float targetScale = isSelected_ ? kSelectedCardScale : 1.0f;
	const EaseType easeType = isSelected_ ? EaseType::EaseOutBack : EaseType::EaseOutCubic;
	currentScale_ = Easing::Lerp(
		scaleTransitionStart_,
		targetScale,
		transitionProgress,
		easeType);

	float displayScale = currentScale_;
	if (isSelected_ && scaleTransitionTimer_.IsFinished()) {
		if (!selectedPulseTimer_.IsActive()) {
			selectedPulseTimer_.Start(kSelectedCardPulseDuration, true);
		}
		selectedPulseTimer_.Update(safeDeltaTime);
		const float pulseAngle = selectedPulseTimer_.GetProgress() *
			2.0f * std::numbers::pi_v<float>;
		displayScale += std::sin(pulseAngle) * kSelectedCardPulseScale;
	}

	ApplySelectionScale(displayScale);
	border->SetColor(isSelected_
		? Vector4{ 1.0f, 0.82f, 0.18f, 1.0f }
		: Vector4{ 0.35f, 0.38f, 0.45f, 1.0f });
}

void WeaponUpgradeCardUI::ApplyLayout() {
	for (MadoEngine::SpriteHandle handle : cardSprites_) {
		if (Sprite* sprite = ResolveSprite(handle)) {
			sprite->SetAnchorPoint({ 0.5f, 0.5f });
		}
	}
	if (Sprite* border = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)])) {
		border->SetColor({ 0.35f, 0.38f, 0.45f, 1.0f });
	}
	if (Sprite* background = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Background)])) {
		background->SetColor({ 0.055f, 0.07f, 0.11f, 0.96f });
	}
	if (Sprite* iconBackground = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBackground)])) {
		iconBackground->SetColor({ 0.025f, 0.03f, 0.05f, 1.0f });
	}
	if (Sprite* cardIconSprite = ResolveSprite(cardIconSprite_)) {
		cardIconSprite->SetAnchorPoint({ 0.5f, 0.5f });
	}
	if (MadoEngine::Text* weaponNameText = ResolveText(weaponNameText_)) {
		weaponNameText->SetFontFamily("Yu Gothic UI");
		weaponNameText->SetFontSize(24.0f);
		weaponNameText->SetAreaSize({ 125.0f, 60.0f });
		weaponNameText->SetAnchorPoint({ 0.5f, 0.5f });
		weaponNameText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		weaponNameText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		weaponNameText->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	if (MadoEngine::Text* categoryText = ResolveText(categoryText_)) {
		categoryText->SetFontFamily("Yu Gothic UI");
		categoryText->SetFontSize(17.0f);
		categoryText->SetAreaSize({ 125.0f, 70.0f });
		categoryText->SetAnchorPoint({ 0.5f, 0.5f });
		categoryText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		categoryText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
	}
	if (MadoEngine::Text* detailText = ResolveText(detailText_)) {
		detailText->SetFontFamily("Yu Gothic UI");
		detailText->SetFontSize(22.0f);
		detailText->SetAreaSize({ 210.0f, 125.0f });
		detailText->SetAnchorPoint({ 0.5f, 0.5f });
		detailText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		detailText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		detailText->SetColor({ 0.92f, 0.94f, 1.0f, 1.0f });
	}
	if (MadoEngine::Text* selectionText = ResolveText(selectionText_)) {
		selectionText->SetFontFamily("Yu Gothic UI");
		selectionText->SetFontSize(20.0f);
		selectionText->SetAreaSize({ 150.0f, 36.0f });
		selectionText->SetAnchorPoint({ 0.5f, 0.5f });
		selectionText->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
		selectionText->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		selectionText->SetColor({ 1.0f, 0.82f, 0.18f, 1.0f });
	}
	ApplySelectionScale(1.0f);
}

void WeaponUpgradeCardUI::ApplySelectionScale(float scale) {
	const float cardPositionX = kCardBasePositionX + kCardPositionDifferenceX * static_cast<float>(cardIndex_);
	const Vector2 cardPosition = { cardPositionX, kCardPositionY };
	const Vector2 iconPosition = ScalePositionAroundCenter(
		{ cardPositionX - 70.0f, 225.0f }, cardPosition, scale);

	if (Sprite* border = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Border)])) {
		border->SetPosition(cardPosition);
		border->SetScale(kCardBorderScale * scale);
	}
	if (Sprite* background = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Background)])) {
		background->SetPosition(cardPosition);
		background->SetScale(kCardBackgroundScale * scale);
	}
	if (Sprite* iconBorder = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBorder)])) {
		iconBorder->SetPosition(iconPosition);
		iconBorder->SetScale(kIconBorderScale * scale);
	}
	if (Sprite* iconBackground = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::IconBackground)])) {
		iconBackground->SetPosition(iconPosition);
		iconBackground->SetScale(kIconBackgroundScale * scale);
	}
	if (Sprite* cardIconSprite = ResolveSprite(cardIconSprite_)) {
		cardIconSprite->SetPosition(iconPosition);
		cardIconSprite->SetScale(kIconScale * scale);
	}

	const Vector2 textScale = { scale, scale };
	if (MadoEngine::Text* weaponNameText = ResolveText(weaponNameText_)) {
		weaponNameText->SetPosition(ScalePositionAroundCenter(
			{ cardPositionX + 38.0f, 205.0f }, cardPosition, scale));
		weaponNameText->SetScale(textScale);
	}
	if (MadoEngine::Text* categoryText = ResolveText(categoryText_)) {
		categoryText->SetPosition(ScalePositionAroundCenter(
			{ cardPositionX + 38.0f, 240.0f }, cardPosition, scale));
		categoryText->SetScale(textScale);
	}
	if (MadoEngine::Text* detailText = ResolveText(detailText_)) {
		detailText->SetPosition(ScalePositionAroundCenter(
			{ cardPositionX, 390.0f }, cardPosition, scale));
		detailText->SetScale(textScale);
	}
	if (MadoEngine::Text* selectionText = ResolveText(selectionText_)) {
		selectionText->SetPosition(ScalePositionAroundCenter(
			{ cardPositionX, 515.0f }, cardPosition, scale));
		selectionText->SetScale(textScale);
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
