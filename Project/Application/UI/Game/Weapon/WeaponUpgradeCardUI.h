#pragma once
#include <RenderHeaders.h>
#include <string>
#include <vector>

namespace UI::Game {

	class UpgradeCardUI {
	public:

		void Initialize();

		void Update(float deltaTime);

	private:
		int selectedCardIndex_ = 0;
		std::vector<Sprite*> cardSprite_;
		Sprite* cardIconSprite_ = nullptr;
	};
}