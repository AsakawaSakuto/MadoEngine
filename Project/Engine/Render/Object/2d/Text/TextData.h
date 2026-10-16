#pragma once
#include "Render/Object/2d/Sprite/SpriteData.h"
#include <cstddef>

namespace MadoEngine {

/// @brief Sprite共通VSとText専用PSで利用する文字と縁取りの描画設定
struct TextMaterial {
	Vector4 color;
	Matrix4x4 uvTransformMatrix;
	Vector4 outlineColor;
	Vector2 textureSize;
	float sdfRange = 0.0f;
	float outlineWidth = 0.0f;
};

// Sprite共通VSのb0と先頭の配置を一致させ、Spriteの描画設定への依存拡大を防止
static_assert(offsetof(TextMaterial, color) == offsetof(SpriteMaterial, color));
static_assert(offsetof(TextMaterial, uvTransformMatrix) == offsetof(SpriteMaterial, uvTransformMatrix));
static_assert(sizeof(TextMaterial) == 112);

} // namespace MadoEngine
