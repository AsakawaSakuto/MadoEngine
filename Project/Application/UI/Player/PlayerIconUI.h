#pragma once
#include <RenderHeaders.h>
#include "../../GameObject/Player/PlayerStatus.h"

namespace Player {
	
	/// @brief プレイヤーアイコンの表示を管理するクラス
	class PlayerIconUI {
	public:

		/// @brief プレイヤーアイコンUIを初期化
		void Initialize(Player::Type type = Player::Type::Gunman);

	private:
		Sprite* playerIcon_;
		Sprite* playerIconFrame_;
	};
}
