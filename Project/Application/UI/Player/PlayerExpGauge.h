#pragma once
#include "RenderHeaders.h"

namespace UI {

	class ExpGauge {
	public:

		void Initialize();

		void Update(float currentExp, float maxExp);

		void DrawImGui();
	private:

		std::unique_ptr<Gauge> expGauge_;

	};
}