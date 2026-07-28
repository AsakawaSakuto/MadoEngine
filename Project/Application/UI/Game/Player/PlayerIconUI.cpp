#include "PlayerIconUI.h"

namespace UI::Game {

	void PlayerIconUI::Initialize(Player::Type type) {
		
		playerIconBG_ = MySprite::Create("PlayerIconBG", "IconFrameBG", SceneType::Game);
		playerIconBG_->SetPosition(Vector2{ 32.0f, 200.0f });

		playerIconFrame_ = MySprite::Create("PlayerIconFrame", "IconFrame", SceneType::Game);
		playerIconFrame_->SetPosition(Vector2{ 32.0f, 200.0f });

		playerIcon_ = MySprite::Create("PlayerIcon", ToTypeText(type), SceneType::Game);
		playerIcon_->SetPosition(Vector2{ 32.0f, 200.0f });
	}

}
