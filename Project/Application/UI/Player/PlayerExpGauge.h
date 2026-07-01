#pragma once
#include "RenderHeaders.h"

namespace Player {

	class ExpGauge
	{
	public:

		void Initialize();

		void Update(float currentExp, float maxExp);

		void DrawImGui();
	private:

		std::unique_ptr<Gauge> expGauge_;

	};
}