#include "ResultWeaponStatsUI.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace {
	constexpr float kFirstRowY = 190.0f;
	constexpr float kRowSpacing = 70.0f;
	constexpr float kIconSize = 52.0f;
	constexpr float kRowHeight = 56.0f;
	constexpr float kTableHeight = 420.0f;
}

namespace UI::Result {

	void WeaponStatsUI::Initialize(const System::PlayLogData& result) {
		std::vector<const System::PlayWeaponRecord*> equippedWeapons;

		// 取り外した武器の履歴を除き、終了時に装備していた武器だけを表示対象に限定
		for (const System::PlayWeaponRecord& weapon : result.weapons) {
			if (weapon.equippedAtEnd) {
				equippedWeapons.push_back(&weapon);
			}
		}
		std::sort(equippedWeapons.begin(), equippedWeapons.end(),
			[](const System::PlayWeaponRecord* left, const System::PlayWeaponRecord* right) {

				// 撃破数が多い武器を優先し、同数の場合は装備スロット順に固定
				if (left->combat.killCount != right->combat.killCount) {
					return left->combat.killCount > right->combat.killCount;
				}
				return left->slotIndex < right->slotIndex;
			});

		// ダメージだけの状態異常も表示し、未発生の状態異常は行数から除外
		const bool hasPoisonStats = result.poison.killCount > 0 || result.poison.damage > 0.0;
		const bool hasBurnStats = result.burn.killCount > 0 || result.burn.damage > 0.0;
		std::size_t rowCount = equippedWeapons.size();
		if (hasPoisonStats) {
			++rowCount;
		}
		if (hasBurnStats) {
			++rowCount;
		}

		// 武器6枠と状態異常2行を画面内へ収め、少ない行数では従来の間隔を維持
		rowSpacing_ = kRowSpacing;
		if (rowCount > 0) {
			rowSpacing_ = (std::min)(kRowSpacing, kTableHeight / static_cast<float>(rowCount));
		}
		CreateText("ResultWeaponHeading", "武器・状態異常", { 220.0f, 120.0f }, { 330.0f, 48.0f }, 28.0f);
		CreateText("ResultKillHeading", "撃破数", { 570.0f, 120.0f }, { 180.0f, 48.0f }, 28.0f,
			MadoEngine::TextHorizontalAlign::Right);
		CreateText("ResultDamageHeading", "与ダメージ", { 800.0f, 120.0f }, { 260.0f, 48.0f }, 28.0f,
			MadoEngine::TextHorizontalAlign::Right);
		for (std::size_t rowIndex = 0; rowIndex < equippedWeapons.size(); ++rowIndex) {
			const System::PlayWeaponRecord& weapon = *equippedWeapons[rowIndex];
			CreateCombatRow("ResultWeapon" + std::to_string(rowIndex),
				Projectile::ProjectileTypeToString(weapon.weaponType),
				std::format("{}  Lv.{}", weapon.name, weapon.level), weapon.combat, rowIndex);
		}

		// 状態異常の独立集計を武器へ加算せず、装備一覧の末尾へ個別に表示
		std::size_t statusRowIndex = equippedWeapons.size();
		if (hasPoisonStats) {
			CreateCombatRow("ResultPoison", "Poison", "毒", result.poison, statusRowIndex);
			++statusRowIndex;
		}
		if (hasBurnStats) {
			CreateCombatRow("ResultBurn", "Burn", "火傷", result.burn, statusRowIndex);
		}

		// Resultから直接起動した場合も架空の装備や前回の数値を生成しない空表示
		if (rowCount == 0) {
			CreateText("ResultEmptyWeapons", "戦績はありません",
				{ 220.0f, kFirstRowY }, { 840.0f, kRowHeight }, 24.0f);
		}
		CreateText("ResultWeaponPageHint", "A / Space：スコアへ",
			{ 220.0f, 620.0f }, { 840.0f, 32.0f }, 20.0f, MadoEngine::TextHorizontalAlign::Center);
	}

	void WeaponStatsUI::SetVisible(bool isVisible) {

		// 表示対象の全Handleへ適用し、毒と火傷や空表示もページ切替時に残さない制御
		for (MadoEngine::SpriteHandle handle : icons_) {
			if (Sprite* icon = MySprite::TryGet(handle)) {
				icon->SetRuntimeVisible(isVisible);
			}
		}
		for (MadoEngine::TextHandle handle : texts_) {
			if (MadoEngine::Text* text = MyText::TryGet(handle)) {
				text->SetRuntimeVisible(isVisible);
			}
		}
	}

	void WeaponStatsUI::CreateCombatRow(const std::string& prefix, const std::string& textureName,
		const std::string& displayName, const System::PlayCombatStats& combat, std::size_t rowIndex) {
		const float rowY = kFirstRowY + static_cast<float>(rowIndex) * rowSpacing_;
		const float rowHeight = (std::min)(kRowHeight, rowSpacing_ - 4.0f);
		const float iconSize = (std::min)(kIconSize, rowHeight - 4.0f);
		const MadoEngine::SpriteHandle iconHandle = MySprite::Create(
			prefix + "Icon", textureName, SceneType::Result, MadoEngine::Render::RenderLayer::UI);
		icons_.push_back(iconHandle);
		if (Sprite* icon = MySprite::TryGet(iconHandle)) {
			icon->SetPosition({ 220.0f, rowY + rowHeight * 0.5f });
			icon->SetAnchorPoint({ 0.0f, 0.5f });
			const Vector2 textureSize = icon->GetTextureSize();
			if (textureSize.x > 0.0f && textureSize.y > 0.0f) {

				// 元画像の縦横比を維持し、行数に応じたアイコン領域へ収容
				const float scale = iconSize / (std::max)(textureSize.x, textureSize.y);
				icon->SetScale({ scale, scale });
			}
		}

		// 終了確定済みの実ダメージを表示し、小数部の丸めによる過大表示を回避
		CreateText(prefix + "Name", displayName,
			{ 290.0f, rowY }, { 270.0f, rowHeight }, 24.0f);
		CreateText(prefix + "Kills", std::format("{}", combat.killCount),
			{ 570.0f, rowY }, { 180.0f, rowHeight }, 28.0f, MadoEngine::TextHorizontalAlign::Right);
		CreateText(prefix + "Damage", std::format("{:.0f}", std::floor(combat.damage)),
			{ 800.0f, rowY }, { 260.0f, rowHeight }, 28.0f, MadoEngine::TextHorizontalAlign::Right);
	}

	void WeaponStatsUI::CreateText(const std::string& name, const std::string& value,
		const Vector2& position, const Vector2& areaSize, float fontSize, MadoEngine::TextHorizontalAlign align) {
		const MadoEngine::TextHandle handle = MyText::Create(
			name, value, SceneType::Result, MadoEngine::Render::RenderLayer::UI);
		texts_.push_back(handle);
		if (MadoEngine::Text* text = MyText::TryGet(handle)) {
			text->SetFontAsset("Assets/Font/YuGothB.ttc", "游ゴシック");
			text->SetFontSize(fontSize);
			text->SetAnchorPoint({ 0.0f, 0.0f });
			text->SetPosition(position);
			text->SetAreaSize(areaSize);
			text->SetHorizontalAlign(align);
			text->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
			text->SetWordWrap(false);
		}
	}

	void WeaponStatsUI::Finalize() {

		// 描画リソースの破棄はSceneManagerのScene単位の寿命管理へ委譲
		icons_.clear();
		texts_.clear();
	}

}
