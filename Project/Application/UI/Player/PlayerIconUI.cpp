#include "PlayerIconUI.h"

namespace Player {

	void PlayerIconUI::Initialize(Player::Type type) {
		
		playerIconFrame_ = MySprite::Create("PlayerIconFrame", "IconFrame", SceneType::Test);
		playerIcon_ = MySprite::Create("PlayerIcon", ToTypeText(type), SceneType::Test);
	}

}