#pragma once
#include "RenderHeaders.h"

namespace UI::Game {

	/// @brief Playerの壁登り可能時間を表示するゲージ
	class PlayerStaminaGauge final {
	public:
		/// @brief 壁登り可能時間ゲージを初期化
		void Initialize();

		/// @brief 壁登りゲージ全体の表示許可を設定
		/// @param isVisible 表示を許可する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief 残り壁登り時間をゲージへ反映
		/// @param remainingTime 壁登り可能な残り時間
		/// @param maxDuration 壁登り可能な最大時間
		/// @param isVisible ゲージを表示する場合はtrue
		void Update(float remainingTime, float maxDuration, bool isVisible);

		/// @brief Manager管理の壁登り可能時間ゲージへの参照を解放
		void Finalize();

	private:
		/// @brief 永続IDから最新のゲージを一時取得
		/// @return 接続中のゲージ、削除済みの場合はnullptr
		Gauge2d* TryGetGauge() const;

		std::string gaugeId_;
		bool isVisible_ = true;
		bool isClimbing_ = false;
	};

} // namespace UI::Game
