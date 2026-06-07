#pragma once

enum class ModelType {
	Static,   // 静的モデル（アニメーションなし）
	Animated, // アニメーションモデル（スキニングなし）
	Skining,  // スキニングモデル（スキニングのみ、アニメーションなし）

	Count
};