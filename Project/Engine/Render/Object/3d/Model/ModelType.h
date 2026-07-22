#pragma once

enum class ModelType {
	Static,   // 静的モデル（アニメーションなし）
	Animated, // アニメーションモデル（スキニングなし）
	Skinning,  // スキニングモデル（スキニングのみ、アニメーションなし）

	Auto // モデルデータから自動判定
};
