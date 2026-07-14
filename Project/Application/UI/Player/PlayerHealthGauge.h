#pragma once
#include "RenderHeaders.h"

namespace Player {

	class HealthGauge {
	public:

		void Initialize();

		void Update(float currentExp, float maxExp);

		void DrawImGui();
	private:

		std::unique_ptr<Gauge> healthGauge_;

	};
}