#include "PlayerIconUI.h"

namespace UI{

	void PlayerIconUI::Initialize(Player::Type type) {
		
		playerIconFrame_ = MySprite::Create("PlayerIconFrame", "IconFrame", SceneType::Game);
		playerIconFrame_->SetPosition(Vector2{ 32.0f, 200.0f });

		playerIcon_ = MySprite::Create("PlayerIcon", ToTypeText(type), SceneType::Game);
		playerIcon_->SetPosition(Vector2{ 32.0f, 200.0f });
	}

}