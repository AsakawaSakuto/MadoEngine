#include "WeaponSelectIcon.h"
#include <string>

namespace UI::Title {

	void WeaponSelectIcon::Initialize(Projectile::Type weaponType, const Vector2& position) {
		const std::string resourceName = Projectile::ProjectileTypeToString(weaponType);
		const std::string prefix = "TitleWeaponSelect" + resourceName;
		const MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::UI;

		// 枠、背景、武器の順に登録して選択色の表示領域を外周へ限定
		frameSprite_ = MySprite::Create(prefix + "Frame", "white16x16", SceneType::Title, layer);
		backgroundSprite_ = MySprite::Create(prefix + "Background", "white16x16", SceneType::Title, layer);
		iconSprite_ = MySprite::Create(prefix + "Icon", resourceName, SceneType::Title, layer);
		for (MadoEngine::SpriteHandle handle : { frameSprite_, backgroundSprite_, iconSprite_ }) {
			if (Sprite* sprite = MySprite::TryGet(handle)) {
				sprite->SetAnchorPoint({ 0.5f, 0.5f });
				sprite->SetPosition(position);
			}
		}
		if (Sprite* background = MySprite::TryGet(backgroundSprite_)) {
			background->SetScale({ 5.5f, 5.5f });
			background->SetColor({ 0.055f, 0.07f, 0.11f, 1.0f });
		}

		nameText_ = MyText::Create(prefix + "Name",
			Projectile::ProjectileTypeToDisplayName(weaponType), SceneType::Title, layer);
		if (MadoEngine::Text* name = MyText::TryGet(nameText_)) {
			name->SetFontAsset("Assets/Font/YuGothB.ttc", "游ゴシック");
			name->SetFontSize(18.0f);
			name->SetAnchorPoint({ 0.5f, 0.5f });
			name->SetPosition({ position.x, position.y + 62.0f });
			name->SetAreaSize({ 120.0f, 32.0f });
			name->SetHorizontalAlign(MadoEngine::TextHorizontalAlign::Center);
			name->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
		}
		SetSelected(false);
	}

	void WeaponSelectIcon::SetSelected(bool isSelected) {
		Vector4 frameColor = { 0.32f, 0.36f, 0.45f, 1.0f };
		Vector4 nameColor = { 0.75f, 0.79f, 0.86f, 1.0f };
		float iconSize = 64.0f;
		float frameScale = 6.0f;

		// 色と大きさを併用して選択位置の視認性を確保
		if (isSelected) {
			frameColor = { 1.0f, 0.82f, 0.18f, 1.0f };
			nameColor = frameColor;
			iconSize = 76.0f;
			frameScale = 6.5f;
		}
		if (Sprite* frame = MySprite::TryGet(frameSprite_)) {
			frame->SetColor(frameColor);
			frame->SetScale({ frameScale, frameScale });
		}
		if (Sprite* icon = MySprite::TryGet(iconSprite_)) {
			const Vector2& textureSize = icon->GetTextureSize();
			if (textureSize.x > 0.0f && textureSize.y > 0.0f) {

				// 素材ごとの解像度差に依存しない画面上のアイコン寸法へ変換
				icon->SetScale({ iconSize / textureSize.x, iconSize / textureSize.y });
			}
		}
		if (MadoEngine::Text* name = MyText::TryGet(nameText_)) {
			name->SetColor(nameColor);
		}
	}

	void WeaponSelectIcon::Finalize() {

		// 描画リソースの破棄はSceneManagerのScene単位の寿命管理へ委譲
		frameSprite_ = {};
		backgroundSprite_ = {};
		iconSprite_ = {};
		nameText_ = {};
	}
}
