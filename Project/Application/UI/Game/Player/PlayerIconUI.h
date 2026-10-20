#pragma once
#include <RenderHeaders.h>
#include "GameObject/Player/PlayerStatus.h"

namespace UI::Game {
	
	/// @brief プレイヤーアイコンの表示を管理するクラス
	class PlayerIconUI {
	public:

		/// @brief プレイヤーアイコンUIを初期化
		void Initialize(Player::Type type = Player::Type::Gunman);

		/// @brief プレイヤーアイコンと背景と枠の表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

	private:
		MadoEngine::SpriteHandle playerIcon_{};
		MadoEngine::SpriteHandle playerIconBG_{};
		MadoEngine::SpriteHandle playerIconFrame_{};
	};
}
