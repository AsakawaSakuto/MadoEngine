#pragma once
#include "RenderHeaders.h"

namespace UI::Game {

	/// @brief Playerの壁登り可能時間を表示するゲージ
	class PlayerStaminaGauge final {
	public:
		/// @brief 壁登り可能時間ゲージを初期化
		void Initialize();

		/// @brief 残り壁登り時間をゲージへ反映
		/// @param remainingTime 壁登り可能な残り時間
		/// @param maxDuration 壁登り可能な最大時間
		/// @param isVisible ゲージを表示する場合はtrue
		void Update(float remainingTime, float maxDuration, bool isVisible);

		/// @brief 壁登り可能時間ゲージを解放
		void Finalize();

		/// @brief 壁登り可能時間ゲージのImGuiを描画
		void DrawImGui();

	private:
		std::unique_ptr<Gauge2d> staminaGauge_;
	};

} // namespace UI::Game
