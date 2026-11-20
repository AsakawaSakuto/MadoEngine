#pragma once
#include "RenderHeaders.h"

namespace UI::Game {

	class PlayerExpGauge {
	public:

		void Initialize();

		/// @brief 経験値ゲージ全体の表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		void Update(float currentExp, float maxExp);

		void IsUpgrade(bool isUpgrade, float deltaTime);
	private:
		/// @brief 永続IDから最新のゲージを一時取得
		/// @return 接続中のゲージ、削除済みの場合はnullptr
		Gauge2d* TryGetGauge() const;

		std::string gaugeId_;
		MadoEngine::SpriteHandle dopaGauge_{};

		float uvOffsetX_ = 0.0f;
		float uvSpeed_ = 4.0f;
		bool isVisible_ = true;
		bool isUpgrading_ = false;
	};
}
