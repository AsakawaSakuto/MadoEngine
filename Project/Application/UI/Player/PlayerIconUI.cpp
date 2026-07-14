#include "PlayerIconUI.h"

namespace Player {

	void PlayerIconUI::Initialize(Player::Type type) {
		
		playerIconFrame_ = MySprite::Create("PlayerIconFrame", "IconFrame", SceneType::Test);
		playerIconFrame_->SetPosition(Vector2{ 32.0f, 200.0f });

		playerIcon_ = MySprite::Create("PlayerIcon", ToTypeText(type), SceneType::Test);
		playerIcon_->SetPosition(Vector2{ 32.0f, 200.0f });
	}

}