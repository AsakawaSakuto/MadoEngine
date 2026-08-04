#include "PlayerIconUI.h"

namespace UI::Game {

	void PlayerIconUI::Initialize(Player::Type type) {
		
		playerIconBG_ = MySprite::Create("PlayerIconBG", "white2x2", SceneType::Game);
		playerIconBG_->SetPosition(Vector2{ 32.0f, 200.0f - 64.0f });
		playerIconBG_->SetScale(Vector2{ 32.0f, 32.0f });
		playerIconBG_->SetColor(Vector4{ 0.1f, 0.1f, 0.1f, 1.0f });

		playerIconFrame_ = MySprite::Create("PlayerIconFrame", "IconFrame", SceneType::Game);
		playerIconFrame_->SetPosition(Vector2{ 32.0f, 200.0f - 64.0f });

		playerIcon_ = MySprite::Create("PlayerIcon", ToTypeText(type), SceneType::Game);
		playerIcon_->SetPosition(Vector2{ 32.0f, 200.0f - 64.0f });
	}

}
