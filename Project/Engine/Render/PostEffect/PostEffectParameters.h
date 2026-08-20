#pragma once

#include "Math/Vector4.h"
#include "Render/PostEffect/PostEffectType.h"
#include <type_traits>

namespace MadoEngine::Render {

struct alignas(16) BinarizeParameters {
	float threshold = 0.5f;
	float intensity = 1.0f;
	float padding0 = 0.0f;
	float padding1 = 0.0f;
	Vector4 lowColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	Vector4 highColor = { 1.0f, 1.0f, 1.0f, 0.0f };
};

struct alignas(16) BloomParameters {
	float intensity = 0.6f;
	float threshold = 1.0f;
	float radius = 4.0f;
	float softKnee = 0.5f;
	Vector4 bloomColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct alignas(16) BoxFilterParameters {
	float radius = 1.0f;
	float intensity = 1.0f;
	float padding0 = 0.0f;
	float padding1 = 0.0f;
};

struct alignas(16) ChromaticAberrationParameters {
	float offsetPixels = 3.0f;
	float edgeStrength = 1.0f;
	float intensity = 1.0f;
	float padding0 = 0.0f;
	float centerX = 0.5f;
	float centerY = 0.5f;
	float padding1 = 0.0f;
	float padding2 = 0.0f;
};

struct alignas(16) ColorFilterParameters {
	float filterColorR = 1.0f;
	float filterColorG = 0.85f;
	float filterColorB = 0.65f;
	float intensity = 1.0f;
};

struct alignas(16) DepthOfFieldParameters {
	float focusDistance = 300.0f;
	float focusRange = 120.0f;
	float blurRadius = 8.0f;
	float intensity = 1.0f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;
	float foregroundStrength = 1.0f;
	float backgroundStrength = 1.0f;
};

struct alignas(16) DissolveParameters {
	float amount = 0.35f;
	float edgeWidth = 0.06f;
	float edgeIntensity = 1.0f;
	float noiseScale = 2.0f;
	Vector4 edgeColor = { 1.0f, 0.45f, 0.05f, 1.0f };
};

struct alignas(16) FogParameters {
	Vector4 color = { 0.58f, 0.68f, 0.74f, 1.0f };
	float startDistance = 850.0f;
	float endDistance = 1000.0f;
	float density = 1.0f;
	float heightStrength = 0.0f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;
	float padding0 = 0.0f;
	float padding1 = 0.0f;
};

struct alignas(16) FXAAParameters {
	float edgeThreshold = 0.125f;
	float minEdgeThreshold = 0.0312f;
	float searchSpan = 8.0f;
	float intensity = 1.0f;
};

struct alignas(16) GaussianFilterParameters {
	float sigma = 1.6f;
	float radius = 2.0f;
	float intensity = 1.0f;
	float padding = 0.0f;
};

struct alignas(16) DepthOutlineParameters {
	Vector4 color = { 1.0f, 0.85f, 0.15f, 1.0f };
	float thickness = 1.0f;
	float depthSensitivity = 80.0f;
	float edgeThreshold = 0.005f;
	float intensity = 1.0f;
};

struct alignas(16) LensDistortionParameters {
	float distortion = 0.18f;
	float cubicDistortion = 0.04f;
	float zoom = 1.0f;
	float intensity = 1.0f;
	float centerX = 0.5f;
	float centerY = 0.5f;
	float padding0 = 0.0f;
	float padding1 = 0.0f;
};

struct alignas(16) LuminanceOutlineParameters {
	Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
	float thickness = 1.0f;
	float luminanceSensitivity = 4.0f;
	float edgeThreshold = 0.05f;
	float intensity = 1.0f;
};

struct alignas(16) PixelArtParameters {
	float pixelSize = 6.0f;
	float colorSteps = 8.0f;
	float contrast = 1.15f;
	float intensity = 1.0f;
};

struct alignas(16) RadialBlurParameters {
	float intensity = 0.45f;
	float sampleCount = 16.0f;
	float radius = 0.45f;
	float falloff = 1.0f;
	float centerX = 0.5f;
	float centerY = 0.5f;
	float padding0 = 0.0f;
	float padding1 = 0.0f;
};

struct alignas(16) RandomParameters {
	float time = 1.0f;
	float noiseScale = 1.0f;
	float contrast = 1.0f;
	float intensity = 1.0f;
};

struct alignas(16) SplitToningParameters {
	float shadowColorR = 0.12f;
	float shadowColorG = 0.25f;
	float shadowColorB = 0.75f;
	float shadowAmount = 0.45f;
	float highlightColorR = 1.0f;
	float highlightColorG = 0.72f;
	float highlightColorB = 0.35f;
	float highlightAmount = 0.35f;
	float balance = 0.0f;
	float softness = 0.2f;
	float intensity = 1.0f;
	float preserveLuminance = 0.75f;
};

struct alignas(16) ToonParameters {
	float colorSteps = 4.0f;
	float saturation = 1.15f;
	float contrast = 1.1f;
	float intensity = 1.0f;
	float thickness = 1.25f;
	float depthSensitivity = 80.0f;
	float edgeThreshold = 0.005f;
	float edgeIntensity = 1.0f;
	float outlineColorR = 0.02f;
	float outlineColorG = 0.025f;
	float outlineColorB = 0.03f;
	float outlineColorA = 1.0f;
};

struct alignas(16) ToneMappingParameters {
	float exposureEV = 0.0f;
	float reinhardWhitePoint = 4.0f;
	float acesBlend = 1.0f;
	float toneMapperMode = 1.0f;
};

struct alignas(16) VignetteParameters {
	float intensity = 0.8f;
	float innerRadius = 0.35f;
	float outerScale = 2.0f;
	float padding = 0.0f;
	Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
};

template<class T>
struct PostEffectParameterTraits {
	static constexpr bool kIsSupported = false;
};

#define MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(ParameterType, EffectType) \
	template<> \
	struct PostEffectParameterTraits<ParameterType> { \
		static constexpr bool kIsSupported = true; \
		static constexpr PostEffectType kEffectType = PostEffectType::EffectType; \
	}

MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(BinarizeParameters, Binarize);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(BloomParameters, Bloom);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(BoxFilterParameters, BoxFilter);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(ChromaticAberrationParameters, ChromaticAberration);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(ColorFilterParameters, ColorFilter);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(DepthOfFieldParameters, DepthOfField);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(DissolveParameters, Dissolve);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(FogParameters, Fog);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(FXAAParameters, FXAA);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(GaussianFilterParameters, GaussianFilter);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(DepthOutlineParameters, DepthOutline);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(LensDistortionParameters, LensDistortion);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(LuminanceOutlineParameters, LuminanceOutline);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(PixelArtParameters, PixelArt);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(RadialBlurParameters, RadialBlur);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(RandomParameters, Random);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(SplitToningParameters, SplitToning);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(ToonParameters, Toon);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(ToneMappingParameters, ToneMapping);
MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS(VignetteParameters, Vignette);

#undef MADOENGINE_DEFINE_POST_EFFECT_PARAMETER_TRAITS

#define MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(ParameterType, ExpectedSize) \
	static_assert(std::is_trivially_copyable_v<ParameterType>); \
	static_assert(sizeof(ParameterType) == ExpectedSize)

MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(BinarizeParameters, 48);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(BloomParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(BoxFilterParameters, 16);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(ChromaticAberrationParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(ColorFilterParameters, 16);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(DepthOfFieldParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(DissolveParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(FogParameters, 48);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(FXAAParameters, 16);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(GaussianFilterParameters, 16);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(DepthOutlineParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(LensDistortionParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(LuminanceOutlineParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(PixelArtParameters, 16);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(RadialBlurParameters, 32);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(RandomParameters, 16);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(SplitToningParameters, 48);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(ToonParameters, 48);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(ToneMappingParameters, 16);
MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS(VignetteParameters, 32);

#undef MADOENGINE_VALIDATE_POST_EFFECT_PARAMETERS

} // namespace MadoEngine::Render
