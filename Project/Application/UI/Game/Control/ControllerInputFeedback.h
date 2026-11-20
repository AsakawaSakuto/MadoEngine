#pragma once
#include "Math/Vector3.h"
#include "Render/Object/ObjectHandle.h"
#include <array>

namespace UI::Game {

	/// @brief Controller Groupのボタンとスティックおよび十字キーの入力表示
	class ControllerInputFeedback final {
	public:
		/// @brief ゲームパッドの押下状態とスティックおよび十字キーの方向へ表示を同期
		void Update();

		/// @brief ボタンの色とスティックおよび十字キーの位置を復元して参照を解除
		void Reset();

	private:
		/// @brief 押下中のボタンと復元用のRGB
		struct ButtonState {
			MadoEngine::SpriteHandle sprite{};
			Vector3 baseColor{};
		};

		/// @brief 押下前のRGBを復元してボタンの参照を解除
		/// @param state 復元対象の押下状態
		void RestoreButton(ButtonState& state);

		std::array<ButtonState, 6> buttons_{};
		std::array<MadoEngine::SpriteHandle, 3> directionalSprites_{};
	};
}
