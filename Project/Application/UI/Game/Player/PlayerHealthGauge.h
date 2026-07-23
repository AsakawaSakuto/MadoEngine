#pragma once
#include "RenderHeaders.h"

namespace UI::Game {

	class PlayerHealthGauge {
	public:
		/// @brief プレイヤーのHPゲージとHP数値表示を初期化
		void Initialize();

		/// @brief プレイヤーのHPゲージとHP数値表示を更新
		/// @param currentHealth 現在のHP
		/// @param maxHealth 最大HP
		void Update(float currentHealth, float maxHealth);

		/// @brief プレイヤーのHP数値表示を終了
		void Finalize();

		/// @brief HPゲージのImGuiを描画
		void DrawImGui();
	private:
		std::unique_ptr<Gauge> healthGauge_;
		MadoEngine::Text* healthText_ = nullptr;
	};
}
