#include "WeaponUpgradeCardUI.h"

namespace {
	static float cardPositionX_ = 440.0f;                // カードの基準位置X座標
	static float cardPositionX_IndexDifference = 200.0f; // カードのインデックスごとのX座標の差
}

namespace UI::Game {
	void UpgradeCardUI::Initialize() {
		cardSprite_.resize(4);

		for (int i = 0; i < 4; i++) {
			cardSprite_[i] = MySprite::Create("cardSprite" + std::to_string(selectedCardIndex_) + std::to_string(i),
				"white16x16", SceneType::Game);
			cardSprite_[i]->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
		}

		cardSprite_[0]->SetPosition(Vector2{ cardPositionX_         + cardPositionX_IndexDifference * selectedCardIndex_, 360.0f });
		cardSprite_[1]->SetPosition(Vector2{ cardPositionX_         + cardPositionX_IndexDifference * selectedCardIndex_, 360.0f });
		cardSprite_[2]->SetPosition(Vector2{ cardPositionX_ - 70.0f + cardPositionX_IndexDifference * selectedCardIndex_, 225.0f });
		cardSprite_[3]->SetPosition(Vector2{ cardPositionX_ - 70.0f + cardPositionX_IndexDifference * selectedCardIndex_, 225.0f });

		cardSprite_[0]->SetScale(Vector2{ 16.0f, 24.0f });
		cardSprite_[1]->SetScale(Vector2{ 15.0f, 23.0f });
		cardSprite_[2]->SetScale(Vector2{ 4.8f, 4.8f });
		cardSprite_[3]->SetScale(Vector2{ 4.0f, 4.0f });

		cardIconSprite_ = MySprite::Create("cardIconSprite" + std::to_string(selectedCardIndex_),
			"white16x16", SceneType::Game);

		cardIconSprite_->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
		cardIconSprite_->SetPosition(Vector2{ cardPositionX_ - 70.0f + cardPositionX_IndexDifference * selectedCardIndex_, 225.0f });
	}
	void UpgradeCardUI::Update(float deltaTime) {
		// 更新処理をここに記述
	}
} // namespace UI::Game