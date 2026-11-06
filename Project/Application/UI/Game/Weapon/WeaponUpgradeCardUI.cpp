#include "WeaponUpgradeCardUI.h"
#include "Core/TextureManager/TextureManager.h"
#include "GameObject/Weapon/WeaponUpgradeSystem.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <utility>

namespace {

constexpr std::array<const char*, 3> kCardGroupNames = { "LeftCard", "CenterCard", "RightCard" };
constexpr std::array<const char*, 5> kCardSpriteNames = {
	"Border", "Background", "LegendaryBackground", "IconBorder", "IconBackground"
};
constexpr float kSelectedCardScale = 1.08f;
constexpr float kSelectedCardPulseScale = 0.01f;
constexpr float kScaleTransitionDuration = 0.18f;
constexpr float kSelectedCardPulseDuration = 1.25f;
constexpr float kDecisionAnimationDuration = 0.2f;
constexpr float kDecisionRiseDistance = 64.0f;
constexpr Vector4 kNewWeaponColor = { 0.95f, 0.97f, 1.0f, 1.0f };

/// @brief 配列形式の色をVector4へ変換
/// @param color RGBA色
/// @return Vector4形式のRGBA色
Vector4 ToVector4(const std::array<float, 4>& color) {
	return { color[0], color[1], color[2], color[3] };
}

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

/// @brief 強化値をステータスに適した精度で文字列化
/// @param choice 表示する強化候補
/// @param value 表示する値
/// @return 表示用の強化値文字列
std::string FormatUpgradeValue(const Weapon::UpgradeChoice& choice, float value) {
	if (choice.statType == Weapon::UpgradeStatType::ShotCooldown) {
		return std::format("{:.3f}秒", value);
	}

	// 状態異常値の意味をカード上で判別できるよう割合と時間へ単位を付与
	if (choice.statType == Weapon::UpgradeStatType::BurnApplyChance ||
		choice.statType == Weapon::UpgradeStatType::PoisonApplyChance ||
		choice.statType == Weapon::UpgradeStatType::FrozenApplyChance ||
		choice.statType == Weapon::UpgradeStatType::FrozenSlowRate) {
		return std::format("{:.1f}%", value);
	}
	if (choice.statType == Weapon::UpgradeStatType::BurnDuration ||
		choice.statType == Weapon::UpgradeStatType::PoisonDuration ||
		choice.statType == Weapon::UpgradeStatType::FrozenDuration) {
		return std::format("{:.1f}秒", value);
	}

	return std::format("{:.1f}", value);
}

/// @brief 強化内容をカード用の文字列へ変換
/// @param choice 表示する強化候補
/// @return ステータス名と強化内容を含む文字列
std::string FormatUpgradeDetail(const Weapon::UpgradeChoice& choice) {
	if (choice.statType == Weapon::UpgradeStatType::ShotCooldown && choice.currentValue > 0.0f) {
		const float reductionRate = -choice.calculatedAmount / choice.currentValue * 100.0f;
		return std::format("{}\n{:.1f}%短縮", choice.statDisplayName, reductionRate);
	}

	return choice.statDisplayName;
}

} // namespace

namespace UI::Game {

void WeaponUpgradeCardUI::Initialize(std::size_t cardIndex) {
	cardIndex_ = cardIndex;
	isInitialized_ = true;
	SetVisible(false);
}

void WeaponUpgradeCardUI::Finalize() {
	if (!isInitialized_) {
		return;
	}

	// Editor所有の要素は削除せず、Groupの演出用変換を戻して参照だけを解放
	SetVisible(false);
	cardGroup_ = {};
	content_.reset();
	cardSprites_.fill({});
	cardIconSprite_ = {};
	weaponNameText_ = {};
	categoryText_ = {};
	detailText_ = {};
	newWeaponDescriptionText_ = {};
	currentValueText_ = {};
	valueArrowText_ = {};
	upgradedValueText_ = {};
	selectionText_ = {};
	accentColor_ = { 0.35f, 0.38f, 0.45f, 1.0f };
	isSelected_ = false;
	isOwnedWeaponUpgrade_ = false;
	isLegendary_ = false;
	isInitialized_ = false;
}

void WeaponUpgradeCardUI::SetChoice(const Weapon::UpgradeChoice& choice) {
	RefreshBindings();

	// 前候補の選択演出を持ち越さず、候補の内容だけを再読込に備えて保持
	scaleTransitionTimer_.Reset();
	selectedPulseTimer_.Reset();
	scaleTransitionStart_ = 1.0f;
	currentScale_ = 1.0f;
	isSelected_ = false;
	ResetDecisionAnimation();
	isOwnedWeaponUpgrade_ = choice.choiceType == Weapon::UpgradeChoiceType::OwnedWeaponUpgrade;
	isLegendary_ = isOwnedWeaponUpgrade_ && choice.rarity && *choice.rarity == Rarity::Legendary;
	CardContent content;
	content.textureName = Projectile::ProjectileTypeToString(choice.weaponType);
	content.weaponName = choice.weaponDisplayName;
	content.detail = FormatUpgradeDetail(choice);
	content.newWeaponDescription = choice.weaponDescription;
	content.currentValue = FormatUpgradeValue(choice, choice.currentValue);
	content.upgradedValue = FormatUpgradeValue(choice, choice.currentValue + choice.calculatedAmount);
	if (isOwnedWeaponUpgrade_) {
		accentColor_ = ToVector4(choice.rarityDisplayColor);
		content.category = choice.rarityDisplayName + "\n" + choice.choiceTypeDisplayName;
	} else {
		accentColor_ = kNewWeaponColor;
		content.category = "NEW\n新規武器";
	}
	content_ = std::move(content);
	ApplyChoiceContent();
	SetVisible(true);
}

void WeaponUpgradeCardUI::SetSelected(bool isSelected) {
	if (isSelected_ != isSelected) {

		// 現在Scaleから補間を開始して連続入力時の見た目の跳ねを防止
		scaleTransitionStart_ = currentScale_;
		scaleTransitionTimer_.Start(kScaleTransitionDuration, false);
		selectedPulseTimer_.Reset();
	}
	isSelected_ = isSelected;
	ApplyVisibility();
}

void WeaponUpgradeCardUI::SetVisible(bool isVisible) {
	RefreshBindings();
	isVisible_ = isVisible;
	if (!isVisible_) {

		// 再表示時に前回の選択・決定演出を残さない状態へ復帰
		scaleTransitionTimer_.Reset();
		selectedPulseTimer_.Reset();
		decisionAnimationTimer_.Reset();
		scaleTransitionStart_ = 1.0f;
		currentScale_ = 1.0f;
		decisionOffsetY_ = 0.0f;
		isDecisionAnimationPlaying_ = false;
		ApplySelectionScale(currentScale_);
	}
	ApplyVisibility();
}

void WeaponUpgradeCardUI::Update(float deltaTime) {
	RefreshBindings();
	ApplyVisibility();
	if (!isVisible_) {
		return;
	}

	// Editor再読込後も候補を再生成せず、内容と演出を新しい要素へ同期
	ApplyChoiceContent();
	const float safeDeltaTime = (std::max)(deltaTime, 0.0f);
	scaleTransitionTimer_.Update(safeDeltaTime);
	float targetScale = 1.0f;
	EaseType easeType = EaseType::EaseOutCubic;
	if (isSelected_) {
		targetScale = kSelectedCardScale;
		easeType = EaseType::EaseOutBack;
	}
	currentScale_ = Easing::Lerp(
		scaleTransitionStart_, targetScale, scaleTransitionTimer_.GetProgress(), easeType);
	float displayScale = currentScale_;
	if (isSelected_ && scaleTransitionTimer_.IsFinished() && !isDecisionAnimationPlaying_) {

		// 選択中だけ周期的な微小拡縮を加えてFocusを表現
		if (!selectedPulseTimer_.IsActive()) {
			selectedPulseTimer_.Start(kSelectedCardPulseDuration, true);
		}
		selectedPulseTimer_.Update(safeDeltaTime);
		const float pulseAngle = selectedPulseTimer_.GetProgress() * 2.0f * std::numbers::pi_v<float>;
		displayScale += std::sin(pulseAngle) * kSelectedCardPulseScale;
	}
	if (isDecisionAnimationPlaying_) {

		// 描画要素が編集中に削除されても決定処理が止まらない独立した時間管理
		decisionAnimationTimer_.Update(safeDeltaTime);
		decisionOffsetY_ = Easing::Lerp(
			0.0f, -kDecisionRiseDistance, decisionAnimationTimer_.GetProgress(), EaseType::EaseOutCubic);
	}
	ApplySelectionScale(displayScale);

	// 親の表示更新より前に暗幕との描画順を確定し、Editorで追加した装飾も登録順で前面化
	auto& groups = MadoEngine::UIGroupManager::GetInstance();
	const MadoEngine::UIGroup* group = groups.TryGet(cardGroup_);
	if (group && group->IsVisible()) {
		auto& sprites = MadoEngine::SpriteManager::GetInstance();
		for (const auto& element : groups.GetElements(cardGroup_)) {
			if (element.resolved && element.type == MadoEngine::UIGroupElementType::Sprite) {
				sprites.MoveToFront(element.sprite);
			}
		}
	}
}

void WeaponUpgradeCardUI::PlayDecisionAnimation() {
	if (!isVisible_ || !isSelected_) {
		return;
	}

	// 選択中の微小拡縮を止め、決定後の上昇演出へ移行
	selectedPulseTimer_.Reset();
	decisionAnimationTimer_.Start(kDecisionAnimationDuration, false);
	decisionOffsetY_ = 0.0f;
	isDecisionAnimationPlaying_ = true;
}

void WeaponUpgradeCardUI::ResetDecisionAnimation() {

	// 候補更新時や演出中断時に上昇量を元へ戻し、選択状態の拡大率だけを維持
	decisionAnimationTimer_.Reset();
	decisionOffsetY_ = 0.0f;
	isDecisionAnimationPlaying_ = false;
	ApplySelectionScale(currentScale_);
}

bool WeaponUpgradeCardUI::IsDecisionAnimationFinished() const {
	return isDecisionAnimationPlaying_ && decisionAnimationTimer_.IsFinished();
}

void WeaponUpgradeCardUI::RefreshBindings() {
	cardGroup_ = {};
	cardSprites_.fill({});
	cardIconSprite_ = {};
	weaponNameText_ = {};
	categoryText_ = {};
	detailText_ = {};
	newWeaponDescriptionText_ = {};
	currentValueText_ = {};
	valueArrowText_ = {};
	upgradedValueText_ = {};
	selectionText_ = {};
	if (cardIndex_ >= kCardGroupNames.size()) {
		return;
	}
	auto& groups = MadoEngine::UIGroupManager::GetInstance();
	const auto upgrade = groups.Find("Upgrade", SceneType::Game);

	// 親内の登録名で解決し、他の親にある同名カードや再読込前のHandleの誤参照を防止
	for (const auto& element : groups.GetElements(upgrade)) {
		if (element.resolved && element.type == MadoEngine::UIGroupElementType::Group &&
			element.name == kCardGroupNames[cardIndex_]) {
			cardGroup_ = element.group;
			break;
		}
	}
	const std::array<std::pair<const char*, MadoEngine::TextHandle*>, 8> textTargets = {{
		{ "WeaponName", &weaponNameText_ }, { "Category", &categoryText_ },
		{ "Detail", &detailText_ }, { "NewWeaponDescription", &newWeaponDescriptionText_ },
		{ "CurrentValue", &currentValueText_ }, { "ValueArrow", &valueArrowText_ },
		{ "UpgradedValue", &upgradedValueText_ }, { "Selection", &selectionText_ }
	}};
	for (const auto& element : groups.GetElements(cardGroup_)) {
		if (!element.resolved) {
			continue;
		}
		if (element.type == MadoEngine::UIGroupElementType::Sprite) {
			if (element.name == "Icon") {
				cardIconSprite_ = element.sprite;
			}
			for (std::size_t index = 0; index < cardSprites_.size(); ++index) {
				if (element.name == kCardSpriteNames[index]) {
					cardSprites_[index] = element.sprite;
					break;
				}
			}
		} else if (element.type == MadoEngine::UIGroupElementType::Text) {
			for (const auto& [name, handle] : textTargets) {
				if (element.name == name) {
					*handle = element.text;
					break;
				}
			}
		}
	}
}

void WeaponUpgradeCardUI::ApplyChoiceContent() {
	if (!content_) {
		return;
	}

	// 候補に依存する値だけを更新し、配置・フォント・サイズはEditor設定を維持
	if (Sprite* icon = ResolveSprite(cardIconSprite_)) {

		// 未登録テクスチャの再確認で毎フレーム警告を出さない事前照会
		const auto& textures = MadoEngine::TextureManager::GetInstance();
		uint32_t textureIndex = UINT32_MAX;
		if (textures.TryGetTextureIndex(content_->textureName, textureIndex)) {
			if (icon->GetTextureName() != content_->textureName) {
				(void)icon->SetTexture(content_->textureName);
			}
		} else if (icon->GetTextureName() != "None" && textures.TryGetTextureIndex("None", textureIndex)) {
			(void)icon->SetTexture("None");
		}
	}
	const std::array<std::pair<MadoEngine::TextHandle, const std::string*>, 6> textTargets = {{
		{ weaponNameText_, &content_->weaponName }, { categoryText_, &content_->category },
		{ detailText_, &content_->detail }, { newWeaponDescriptionText_, &content_->newWeaponDescription },
		{ currentValueText_, &content_->currentValue }, { upgradedValueText_, &content_->upgradedValue }
	}};
	for (const auto& [handle, value] : textTargets) {
		if (MadoEngine::Text* text = ResolveText(handle)) {
			text->SetText(*value);
		}
	}
	if (MadoEngine::Text* category = ResolveText(categoryText_)) {
		category->SetColor(accentColor_);
	}
	for (const auto type : { CardSpriteType::Border, CardSpriteType::IconBorder }) {
		if (Sprite* border = ResolveSprite(cardSprites_[static_cast<std::size_t>(type)])) {
			border->SetColor(accentColor_);
		}
	}
}

void WeaponUpgradeCardUI::ApplySelectionScale(float scale) {
	if (MadoEngine::UIGroup* group = MadoEngine::UIGroupManager::GetInstance().TryGet(cardGroup_)) {

		// 毎回編集用Transformを基準に合成し、拡縮の累積とEditor設定の書き換えを防止
		auto transform = group->GetAuthoringState().transform;
		transform.scale *= scale;
		transform.translate.y += decisionOffsetY_;
		group->SetTransform(transform);
	}
}

void WeaponUpgradeCardUI::ApplyVisibility() {
	if (MadoEngine::UIGroup* group = MadoEngine::UIGroupManager::GetInstance().TryGet(cardGroup_)) {
		group->SetVisible(isVisible_ && group->GetAuthoringState().visible);
	}

	// Groupの表示と候補種別の条件を分離し、各要素のEditor表示設定を維持
	if (Sprite* background = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::Background)])) {
		background->SetRuntimeVisible(!isLegendary_);
	}
	if (Sprite* legendary = ResolveSprite(cardSprites_[static_cast<std::size_t>(CardSpriteType::LegendaryBackground)])) {
		legendary->SetRuntimeVisible(isLegendary_);
	}
	if (MadoEngine::Text* detail = ResolveText(detailText_)) {
		detail->SetRuntimeVisible(isOwnedWeaponUpgrade_);
	}
	if (MadoEngine::Text* description = ResolveText(newWeaponDescriptionText_)) {
		description->SetRuntimeVisible(!isOwnedWeaponUpgrade_);
	}
	for (const auto handle : { currentValueText_, valueArrowText_, upgradedValueText_ }) {
		if (MadoEngine::Text* text = ResolveText(handle)) {
			text->SetRuntimeVisible(isOwnedWeaponUpgrade_);
		}
	}
	if (MadoEngine::Text* selection = ResolveText(selectionText_)) {
		selection->SetRuntimeVisible(isSelected_);
	}
}

} // namespace UI::Game
