#pragma once
#include "RenderHeaders.h"

namespace UI::Game {

	class PlayerExpGauge {
	public:

		void Initialize();

		void Update(float currentExp, float maxExp);

		void DrawImGui();

		void IsUpgrade(bool isUpgrade, float deltaTime);
	private:

		std::unique_ptr<Gauge> expGauge_;
		MadoEngine::SpriteHandle dopaGauge_{};

		float uvOffsetX_ = 0.0f;
		float uvSpeed_ = 4.0f;
	};
}
