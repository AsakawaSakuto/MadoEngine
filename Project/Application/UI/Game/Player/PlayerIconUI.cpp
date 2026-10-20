#include "PlayerIconUI.h"

namespace UI::Game {

	void PlayerIconUI::SetVisible(bool isVisible) {

		// Icon本体と装飾を同時に切り替え、背景や枠だけが残る表示を防止
		for (MadoEngine::SpriteHandle handle : { playerIcon_, playerIconBG_, playerIconFrame_ }) {
			if (Sprite* sprite = MySprite::TryGet(handle)) {
				sprite->SetVisible(isVisible);
			}
		}
	}

	void PlayerIconUI::Initialize(Player::Type type) {
		
		playerIconBG_ = MySprite::Create("PlayerIconBG", "white2x2", SceneType::Game);
		if (Sprite* sprite = MySprite::TryGet(playerIconBG_)) {
			sprite->SetPosition(Vector2{ 32.0f, 200.0f - 64.0f });
			sprite->SetScale(Vector2{ 32.0f, 32.0f });
			sprite->SetColor(Vector4{ 0.1f, 0.1f, 0.1f, 1.0f });
		}

		playerIconFrame_ = MySprite::Create("PlayerIconFrame", "IconFrame", SceneType::Game);
		if (Sprite* sprite = MySprite::TryGet(playerIconFrame_)) {
			sprite->SetPosition(Vector2{ 32.0f, 200.0f - 64.0f });
		}

		playerIcon_ = MySprite::Create("PlayerIcon", ToTypeText(type), SceneType::Game);
		if (Sprite* sprite = MySprite::TryGet(playerIcon_)) {
			sprite->SetPosition(Vector2{ 32.0f, 200.0f - 64.0f });
		}
	}

}
