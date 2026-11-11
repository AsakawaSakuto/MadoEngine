#include "PlayerIconUI.h"
#include "Render/Object/2d/Sprite/MySprite.h"

namespace UI::Game {

	void PlayerIconUI::SetVisible(bool isVisible) {
		iconGroup_.SetVisible(isVisible);
	}

	void PlayerIconUI::Initialize(Player::Type type) {

		// 共通の表示位置を親に置き、各Spriteには寸法と見た目だけを設定
		iconGroup_.Clear();
		iconGroup_.SetPosition({ 32.0f, 136.0f });
		const auto background = MySprite::Create("PlayerIconBG", "white2x2", SceneType::Game);
		if (Sprite* sprite = MySprite::TryGet(background)) {
			sprite->SetScale(Vector2{ 32.0f, 32.0f });
			sprite->SetColor(Vector4{ 0.1f, 0.1f, 0.1f, 1.0f });
		}
		iconGroup_.AddSprite("Background", background);

		const auto frame = MySprite::Create("PlayerIconFrame", "IconFrame", SceneType::Game);
		if (Sprite* sprite = MySprite::TryGet(frame)) {
			sprite->SetScale(Vector2{ 4.0f, 4.0f });
		}
		iconGroup_.AddSprite("Frame", frame);

		const auto icon = MySprite::Create("PlayerIcon", ToTypeText(type), SceneType::Game);
		iconGroup_.AddSprite("Icon", icon);
	}

}
