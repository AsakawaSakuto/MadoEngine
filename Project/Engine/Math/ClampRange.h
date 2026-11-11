#pragma once

namespace Math {

	/// @brief クランプ範囲の下限と上限
	/// @note クランプ時の前提条件は `min <= max`
	struct ClampRange {
		float min = 0.0f;
		float max = 0.0f;
	};

}
