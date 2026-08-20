#pragma once

#include <cstdint>

namespace MadoEngine::Render {

/// @brief エンジンが提供するポストエフェクト種別
enum class PostEffectType : uint32_t {
	CopyImage,
	Binarize,
	Bloom,
	BoxFilter,
	ChromaticAberration,
	ColorFilter,
	DepthOfField,
	Dissolve,
	Fog,
	GaussianFilter,
	GrayScale,
	Invert,
	DepthOutline,
	LensDistortion,
	LuminanceOutline,
	PixelArt,
	RadialBlur,
	Random,
	Sepia,
	SplitToning,
	Toon,
	Vignette,
	FXAA,
	ToneMapping,
	CRT,
};

} // namespace MadoEngine::Render
