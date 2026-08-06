#pragma once
#include <RenderHeaders.h>
#include "GameObject/Player/PlayerStatus.h"

namespace UI::Game {
	
	/// @brief プレイヤーアイコンの表示を管理するクラス
	class PlayerIconUI {
	public:

		/// @brief プレイヤーアイコンUIを初期化
		void Initialize(Player::Type type = Player::Type::Gunman);

	private:
		MadoEngine::SpriteHandle playerIcon_{};
		MadoEngine::SpriteHandle playerIconBG_{};
		MadoEngine::SpriteHandle playerIconFrame_{};
	};
}
