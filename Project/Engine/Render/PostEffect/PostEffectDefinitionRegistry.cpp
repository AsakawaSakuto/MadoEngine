#include "Render/PostEffect/PostEffectDefinitionRegistry.h"
#include "Render/PostEffect/PostEffectParameters.h"
#include <array>
#include <cstddef>

namespace MadoEngine::Render {

namespace {

/// @brief Vector型Parameter内の要素Offsetを計算
/// @param vectorOffset 構造体先頭からVector型ParameterまでのOffset
/// @param componentIndex Vector内の要素Index
/// @return 構造体先頭から対象要素までのOffset
constexpr std::size_t VectorComponentOffset(std::size_t vectorOffset, std::size_t componentIndex) {
	return vectorOffset + sizeof(float) * componentIndex;
}

const BinarizeParameters kBinarizeDefaults{};
const BloomParameters kBloomDefaults{};
const BoxFilterParameters kBoxFilterDefaults{};
const ChromaticAberrationParameters kChromaticAberrationDefaults{};
const ColorFilterParameters kColorFilterDefaults{};
const DepthOfFieldParameters kDepthOfFieldDefaults{};
const DissolveParameters kDissolveDefaults{};
const FogParameters kFogDefaults{};
const FXAAParameters kFXAADefaults{};
const GaussianFilterParameters kGaussianFilterDefaults{};
const DepthOutlineParameters kDepthOutlineDefaults{};
const LensDistortionParameters kLensDistortionDefaults{};
const LuminanceOutlineParameters kLuminanceOutlineDefaults{};
const PixelArtParameters kPixelArtDefaults{};
const RadialBlurParameters kRadialBlurDefaults{};
const RandomParameters kRandomDefaults{};
const SplitToningParameters kSplitToningDefaults{};
const ToonParameters kToonDefaults{};
const ToneMappingParameters kToneMappingDefaults{};
const VignetteParameters kVignetteDefaults{};

const PostEffectFloatParameterDefinition kBinarizeParameters[] = {
	{ "Threshold", "しきい値", offsetof(BinarizeParameters, threshold), 0.0f, 1.0f, 0.01f },
	{ "Intensity", "適用率", offsetof(BinarizeParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "LowColorR", "低輝度色R", VectorComponentOffset(offsetof(BinarizeParameters, lowColor), 0), 0.0f, 1.0f, 0.01f },
	{ "LowColorG", "低輝度色G", VectorComponentOffset(offsetof(BinarizeParameters, lowColor), 1), 0.0f, 1.0f, 0.01f },
	{ "LowColorB", "低輝度色B", VectorComponentOffset(offsetof(BinarizeParameters, lowColor), 2), 0.0f, 1.0f, 0.01f },
	{ "HighColorR", "高輝度色R", VectorComponentOffset(offsetof(BinarizeParameters, highColor), 0), 0.0f, 1.0f, 0.01f },
	{ "HighColorG", "高輝度色G", VectorComponentOffset(offsetof(BinarizeParameters, highColor), 1), 0.0f, 1.0f, 0.01f },
	{ "HighColorB", "高輝度色B", VectorComponentOffset(offsetof(BinarizeParameters, highColor), 2), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kBloomParameters[] = {
	{ "Intensity", "強度", offsetof(BloomParameters, intensity), 0.0f, 5.0f, 0.01f },
	{ "Threshold", "HDRしきい値", offsetof(BloomParameters, threshold), 0.0f, 32.0f, 0.01f },
	{ "Radius", "半径", offsetof(BloomParameters, radius), 0.0f, 32.0f, 0.1f },
	{ "SoftKnee", "ソフトニー", offsetof(BloomParameters, softKnee), 0.0f, 1.0f, 0.01f },
	{ "BloomColorR", "発光色R", VectorComponentOffset(offsetof(BloomParameters, bloomColor), 0), 0.0f, 1.0f, 0.01f },
	{ "BloomColorG", "発光色G", VectorComponentOffset(offsetof(BloomParameters, bloomColor), 1), 0.0f, 1.0f, 0.01f },
	{ "BloomColorB", "発光色B", VectorComponentOffset(offsetof(BloomParameters, bloomColor), 2), 0.0f, 1.0f, 0.01f },
	{ "BloomColorA", "発光色A", VectorComponentOffset(offsetof(BloomParameters, bloomColor), 3), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kBoxFilterParameters[] = {
	{ "Radius", "半径", offsetof(BoxFilterParameters, radius), 1.0f, 8.0f, 1.0f },
	{ "Intensity", "適用率", offsetof(BoxFilterParameters, intensity), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kChromaticAberrationParameters[] = {
	{ "OffsetPixels", "ずれ量", offsetof(ChromaticAberrationParameters, offsetPixels), 0.0f, 32.0f, 0.1f },
	{ "EdgeStrength", "外周強度", offsetof(ChromaticAberrationParameters, edgeStrength), 0.001f, 4.0f, 0.01f },
	{ "Intensity", "適用率", offsetof(ChromaticAberrationParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "CenterX", "中心X", offsetof(ChromaticAberrationParameters, centerX), 0.0f, 1.0f, 0.01f },
	{ "CenterY", "中心Y", offsetof(ChromaticAberrationParameters, centerY), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kColorFilterParameters[] = {
	{ "FilterColorR", "フィルター色R", offsetof(ColorFilterParameters, filterColorR), 0.0f, 1.0f, 0.01f },
	{ "FilterColorG", "フィルター色G", offsetof(ColorFilterParameters, filterColorG), 0.0f, 1.0f, 0.01f },
	{ "FilterColorB", "フィルター色B", offsetof(ColorFilterParameters, filterColorB), 0.0f, 1.0f, 0.01f },
	{ "Intensity", "適用率", offsetof(ColorFilterParameters, intensity), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kDepthOfFieldParameters[] = {
	{ "FocusDistance", "焦点距離", offsetof(DepthOfFieldParameters, focusDistance), 0.0f, 5000.0f, 1.0f },
	{ "FocusRange", "焦点幅", offsetof(DepthOfFieldParameters, focusRange), 0.001f, 5000.0f, 1.0f },
	{ "BlurRadius", "ぼかし半径", offsetof(DepthOfFieldParameters, blurRadius), 0.0f, 32.0f, 0.1f },
	{ "Intensity", "適用率", offsetof(DepthOfFieldParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "NearClip", "NearClip", offsetof(DepthOfFieldParameters, nearClip), 0.001f, 100.0f, 0.01f },
	{ "FarClip", "FarClip", offsetof(DepthOfFieldParameters, farClip), 1.0f, 10000.0f, 1.0f },
	{ "ForegroundStrength", "手前ぼけ強度", offsetof(DepthOfFieldParameters, foregroundStrength), 0.0f, 4.0f, 0.01f },
	{ "BackgroundStrength", "奥ぼけ強度", offsetof(DepthOfFieldParameters, backgroundStrength), 0.0f, 4.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kDissolveParameters[] = {
	{ "Amount", "進行度", offsetof(DissolveParameters, amount), 0.0f, 1.0f, 0.01f },
	{ "EdgeWidth", "境界幅", offsetof(DissolveParameters, edgeWidth), 0.001f, 0.5f, 0.001f },
	{ "EdgeIntensity", "境界強度", offsetof(DissolveParameters, edgeIntensity), 0.0f, 8.0f, 0.01f },
	{ "NoiseScale", "ノイズ倍率", offsetof(DissolveParameters, noiseScale), 0.001f, 32.0f, 0.01f },
	{ "EdgeColorR", "境界色R", VectorComponentOffset(offsetof(DissolveParameters, edgeColor), 0), 0.0f, 1.0f, 0.01f },
	{ "EdgeColorG", "境界色G", VectorComponentOffset(offsetof(DissolveParameters, edgeColor), 1), 0.0f, 1.0f, 0.01f },
	{ "EdgeColorB", "境界色B", VectorComponentOffset(offsetof(DissolveParameters, edgeColor), 2), 0.0f, 1.0f, 0.01f },
	{ "EdgeColorA", "境界色A", VectorComponentOffset(offsetof(DissolveParameters, edgeColor), 3), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kFogParameters[] = {
	{ "ColorR", "色R", VectorComponentOffset(offsetof(FogParameters, color), 0), 0.0f, 1.0f, 0.01f },
	{ "ColorG", "色G", VectorComponentOffset(offsetof(FogParameters, color), 1), 0.0f, 1.0f, 0.01f },
	{ "ColorB", "色B", VectorComponentOffset(offsetof(FogParameters, color), 2), 0.0f, 1.0f, 0.01f },
	{ "ColorA", "色A", VectorComponentOffset(offsetof(FogParameters, color), 3), 0.0f, 1.0f, 0.01f },
	{ "StartDistance", "開始距離", offsetof(FogParameters, startDistance), 0.0f, 5000.0f, 1.0f },
	{ "EndDistance", "終了距離", offsetof(FogParameters, endDistance), 0.0f, 5000.0f, 1.0f },
	{ "Density", "濃度", offsetof(FogParameters, density), 0.0f, 4.0f, 0.01f },
	{ "HeightStrength", "高さ強度", offsetof(FogParameters, heightStrength), 0.0f, 4.0f, 0.01f },
	{ "NearClip", "NearClip", offsetof(FogParameters, nearClip), 0.001f, 100.0f, 0.01f },
	{ "FarClip", "FarClip", offsetof(FogParameters, farClip), 1.0f, 10000.0f, 1.0f },
};

const PostEffectFloatParameterDefinition kFXAAParameters[] = {
	{ "EdgeThreshold", "相対エッジしきい値", offsetof(FXAAParameters, edgeThreshold), 0.0312f, 0.333f, 0.001f },
	{ "MinEdgeThreshold", "最小エッジしきい値", offsetof(FXAAParameters, minEdgeThreshold), 0.001f, 0.0833f, 0.001f },
	{ "SearchSpan", "輪郭探索範囲", offsetof(FXAAParameters, searchSpan), 2.0f, 16.0f, 0.5f },
	{ "Intensity", "適用率", offsetof(FXAAParameters, intensity), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kGaussianFilterParameters[] = {
	{ "Sigma", "標準偏差", offsetof(GaussianFilterParameters, sigma), 0.001f, 8.0f, 0.01f },
	{ "Radius", "半径", offsetof(GaussianFilterParameters, radius), 1.0f, 8.0f, 1.0f },
	{ "Intensity", "適用率", offsetof(GaussianFilterParameters, intensity), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kDepthOutlineParameters[] = {
	{ "ColorR", "色R", VectorComponentOffset(offsetof(DepthOutlineParameters, color), 0), 0.0f, 1.0f, 0.01f },
	{ "ColorG", "色G", VectorComponentOffset(offsetof(DepthOutlineParameters, color), 1), 0.0f, 1.0f, 0.01f },
	{ "ColorB", "色B", VectorComponentOffset(offsetof(DepthOutlineParameters, color), 2), 0.0f, 1.0f, 0.01f },
	{ "ColorA", "色A", VectorComponentOffset(offsetof(DepthOutlineParameters, color), 3), 0.0f, 1.0f, 0.01f },
	{ "Thickness", "太さ", offsetof(DepthOutlineParameters, thickness), 0.25f, 12.0f, 0.05f },
	{ "DepthSensitivity", "深度感度", offsetof(DepthOutlineParameters, depthSensitivity), 1.0f, 300.0f, 1.0f },
	{ "EdgeThreshold", "エッジしきい値", offsetof(DepthOutlineParameters, edgeThreshold), 0.0001f, 0.1f, 0.0001f },
	{ "Intensity", "濃さ", offsetof(DepthOutlineParameters, intensity), 0.0f, 4.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kLensDistortionParameters[] = {
	{ "Distortion", "歪み量", offsetof(LensDistortionParameters, distortion), -1.0f, 1.0f, 0.001f },
	{ "CubicDistortion", "二次歪み量", offsetof(LensDistortionParameters, cubicDistortion), -1.0f, 1.0f, 0.001f },
	{ "Zoom", "ズーム", offsetof(LensDistortionParameters, zoom), 0.25f, 4.0f, 0.01f },
	{ "Intensity", "適用率", offsetof(LensDistortionParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "CenterX", "中心X", offsetof(LensDistortionParameters, centerX), 0.0f, 1.0f, 0.01f },
	{ "CenterY", "中心Y", offsetof(LensDistortionParameters, centerY), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kLuminanceOutlineParameters[] = {
	{ "ColorR", "輪郭色R", VectorComponentOffset(offsetof(LuminanceOutlineParameters, color), 0), 0.0f, 1.0f, 0.01f },
	{ "ColorG", "輪郭色G", VectorComponentOffset(offsetof(LuminanceOutlineParameters, color), 1), 0.0f, 1.0f, 0.01f },
	{ "ColorB", "輪郭色B", VectorComponentOffset(offsetof(LuminanceOutlineParameters, color), 2), 0.0f, 1.0f, 0.01f },
	{ "ColorA", "輪郭色A", VectorComponentOffset(offsetof(LuminanceOutlineParameters, color), 3), 0.0f, 1.0f, 0.01f },
	{ "Thickness", "太さ", offsetof(LuminanceOutlineParameters, thickness), 0.25f, 12.0f, 0.05f },
	{ "LuminanceSensitivity", "輝度感度", offsetof(LuminanceOutlineParameters, luminanceSensitivity), 0.1f, 32.0f, 0.01f },
	{ "EdgeThreshold", "エッジしきい値", offsetof(LuminanceOutlineParameters, edgeThreshold), 0.0001f, 0.5f, 0.0001f },
	{ "Intensity", "強さ", offsetof(LuminanceOutlineParameters, intensity), 0.0f, 4.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kPixelArtParameters[] = {
	{ "PixelSize", "ピクセルサイズ", offsetof(PixelArtParameters, pixelSize), 1.0f, 64.0f, 1.0f },
	{ "ColorSteps", "色階調数", offsetof(PixelArtParameters, colorSteps), 2.0f, 32.0f, 1.0f },
	{ "Contrast", "コントラスト", offsetof(PixelArtParameters, contrast), 0.0f, 4.0f, 0.01f },
	{ "Intensity", "適用率", offsetof(PixelArtParameters, intensity), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kRadialBlurParameters[] = {
	{ "Intensity", "強度", offsetof(RadialBlurParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "SampleCount", "サンプル数", offsetof(RadialBlurParameters, sampleCount), 1.0f, 64.0f, 1.0f },
	{ "Radius", "半径", offsetof(RadialBlurParameters, radius), 0.0f, 2.0f, 0.01f },
	{ "Falloff", "距離減衰", offsetof(RadialBlurParameters, falloff), 0.1f, 4.0f, 0.01f },
	{ "CenterX", "中心X", offsetof(RadialBlurParameters, centerX), 0.0f, 1.0f, 0.01f },
	{ "CenterY", "中心Y", offsetof(RadialBlurParameters, centerY), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kRandomParameters[] = {
	{ "Time", "時間", offsetof(RandomParameters, time), 0.0001f, 1000.0f, 0.01f },
	{ "NoiseScale", "ノイズ拡大率", offsetof(RandomParameters, noiseScale), 0.0001f, 256.0f, 0.1f },
	{ "Contrast", "コントラスト", offsetof(RandomParameters, contrast), 0.0f, 8.0f, 0.01f },
	{ "Intensity", "適用率", offsetof(RandomParameters, intensity), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kSplitToningParameters[] = {
	{ "ShadowColorR", "暗部色R", offsetof(SplitToningParameters, shadowColorR), 0.0f, 1.0f, 0.01f },
	{ "ShadowColorG", "暗部色G", offsetof(SplitToningParameters, shadowColorG), 0.0f, 1.0f, 0.01f },
	{ "ShadowColorB", "暗部色B", offsetof(SplitToningParameters, shadowColorB), 0.0f, 1.0f, 0.01f },
	{ "ShadowAmount", "暗部適用量", offsetof(SplitToningParameters, shadowAmount), 0.0f, 1.0f, 0.01f },
	{ "HighlightColorR", "明部色R", offsetof(SplitToningParameters, highlightColorR), 0.0f, 1.0f, 0.01f },
	{ "HighlightColorG", "明部色G", offsetof(SplitToningParameters, highlightColorG), 0.0f, 1.0f, 0.01f },
	{ "HighlightColorB", "明部色B", offsetof(SplitToningParameters, highlightColorB), 0.0f, 1.0f, 0.01f },
	{ "HighlightAmount", "明部適用量", offsetof(SplitToningParameters, highlightAmount), 0.0f, 1.0f, 0.01f },
	{ "Balance", "分岐位置", offsetof(SplitToningParameters, balance), -1.0f, 1.0f, 0.01f },
	{ "Softness", "なじみ幅", offsetof(SplitToningParameters, softness), 0.001f, 1.0f, 0.001f },
	{ "Intensity", "適用率", offsetof(SplitToningParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "PreserveLuminance", "輝度保持率", offsetof(SplitToningParameters, preserveLuminance), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kToonParameters[] = {
	{ "ColorSteps", "色階調数", offsetof(ToonParameters, colorSteps), 2.0f, 12.0f, 1.0f },
	{ "Saturation", "彩度", offsetof(ToonParameters, saturation), 0.0f, 3.0f, 0.01f },
	{ "Contrast", "コントラスト", offsetof(ToonParameters, contrast), 0.0f, 4.0f, 0.01f },
	{ "Intensity", "適用率", offsetof(ToonParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "Thickness", "輪郭太さ", offsetof(ToonParameters, thickness), 0.25f, 12.0f, 0.05f },
	{ "DepthSensitivity", "深度感度", offsetof(ToonParameters, depthSensitivity), 1.0f, 300.0f, 1.0f },
	{ "EdgeThreshold", "輪郭しきい値", offsetof(ToonParameters, edgeThreshold), 0.0001f, 0.1f, 0.0001f },
	{ "EdgeIntensity", "輪郭強度", offsetof(ToonParameters, edgeIntensity), 0.0f, 4.0f, 0.01f },
	{ "OutlineColorR", "輪郭色R", offsetof(ToonParameters, outlineColorR), 0.0f, 1.0f, 0.01f },
	{ "OutlineColorG", "輪郭色G", offsetof(ToonParameters, outlineColorG), 0.0f, 1.0f, 0.01f },
	{ "OutlineColorB", "輪郭色B", offsetof(ToonParameters, outlineColorB), 0.0f, 1.0f, 0.01f },
};

const std::string_view kToneMapperModeOptions[] = {
	"Linear",
	"Neutral",
	"Filmic",
};

const PostEffectFloatParameterDefinition kToneMappingParameters[] = {
	{
		"ToneMapperMode",
		"Tone Mapper",
		offsetof(ToneMappingParameters, toneMapperMode),
		0.0f,
		2.0f,
		1.0f,
		kToneMapperModeOptions,
		3,
	},
	{ "ExposureEV", "露出EV", offsetof(ToneMappingParameters, exposureEV), -10.0f, 10.0f, 0.01f },
	{ "ReinhardWhitePoint", "Filmic Reinhard白レベル", offsetof(ToneMappingParameters, reinhardWhitePoint), 1.0f, 32.0f, 0.1f },
	{ "ACESBlend", "Filmic ACES適用率", offsetof(ToneMappingParameters, acesBlend), 0.0f, 1.0f, 0.01f },
};

const PostEffectFloatParameterDefinition kVignetteParameters[] = {
	{ "Intensity", "強度", offsetof(VignetteParameters, intensity), 0.0f, 1.0f, 0.01f },
	{ "InnerRadius", "内側半径", offsetof(VignetteParameters, innerRadius), 0.0f, 1.0f, 0.01f },
	{ "OuterScale", "外側倍率", offsetof(VignetteParameters, outerScale), 1.0f, 4.0f, 0.01f },
	{ "ColorR", "色R", VectorComponentOffset(offsetof(VignetteParameters, color), 0), 0.0f, 1.0f, 0.01f },
	{ "ColorG", "色G", VectorComponentOffset(offsetof(VignetteParameters, color), 1), 0.0f, 1.0f, 0.01f },
	{ "ColorB", "色B", VectorComponentOffset(offsetof(VignetteParameters, color), 2), 0.0f, 1.0f, 0.01f },
	{ "ColorA", "色A", VectorComponentOffset(offsetof(VignetteParameters, color), 3), 0.0f, 1.0f, 0.01f },
};

/// @brief 既定値と編集Parameterを持つPostEffect定義を作成
/// @param type PostEffect種別
/// @param typeName 種別名
/// @param shaderKey Pixel Shader Key
/// @param defaults Parameter既定値
/// @param parameters 編集可能Parameter定義
/// @return PostEffect定義
template<class T, std::size_t N>
constexpr PostEffectDefinition CreateDefinition(
	PostEffectType type,
	std::string_view typeName,
	std::string_view shaderKey,
	const T& defaults,
	const PostEffectFloatParameterDefinition (&parameters)[N])
{
	return { type, typeName, typeName, shaderKey, &defaults, sizeof(T), parameters, N };
}

/// @brief Parameterを持たないPostEffect定義を作成
/// @param type PostEffect種別
/// @param typeName 種別名
/// @param shaderKey Pixel Shader Key
/// @return PostEffect定義
constexpr PostEffectDefinition CreateParameterlessDefinition(
	PostEffectType type,
	std::string_view typeName,
	std::string_view shaderKey)
{
	return { type, typeName, typeName, shaderKey, nullptr, 0, nullptr, 0 };
}

const std::array kDefinitions = {
	CreateParameterlessDefinition(PostEffectType::CopyImage, "CopyImage", "PostEffect/CopyImage.PS"),
	CreateDefinition(PostEffectType::Binarize, "Binarize", "PostEffect/Binarize.PS", kBinarizeDefaults, kBinarizeParameters),
	CreateDefinition(PostEffectType::Bloom, "Bloom", "PostEffect/Bloom.PS", kBloomDefaults, kBloomParameters),
	CreateDefinition(PostEffectType::BoxFilter, "BoxFilter", "PostEffect/BoxFilter.PS", kBoxFilterDefaults, kBoxFilterParameters),
	CreateDefinition(PostEffectType::ChromaticAberration, "ChromaticAberration", "PostEffect/ChromaticAberration.PS", kChromaticAberrationDefaults, kChromaticAberrationParameters),
	CreateDefinition(PostEffectType::ColorFilter, "ColorFilter", "PostEffect/ColorFilter.PS", kColorFilterDefaults, kColorFilterParameters),
	CreateDefinition(PostEffectType::DepthOfField, "DepthOfField", "PostEffect/DepthOfField.PS", kDepthOfFieldDefaults, kDepthOfFieldParameters),
	CreateDefinition(PostEffectType::Dissolve, "Dissolve", "PostEffect/Dissolve.PS", kDissolveDefaults, kDissolveParameters),
	CreateDefinition(PostEffectType::Fog, "Fog", "PostEffect/Fog.PS", kFogDefaults, kFogParameters),
	CreateDefinition(PostEffectType::GaussianFilter, "GaussianFilter", "PostEffect/GaussianFilter.PS", kGaussianFilterDefaults, kGaussianFilterParameters),
	CreateParameterlessDefinition(PostEffectType::GrayScale, "GrayScale", "PostEffect/GrayScale.PS"),
	CreateParameterlessDefinition(PostEffectType::Invert, "Invert", "PostEffect/Invert.PS"),
	CreateDefinition(PostEffectType::DepthOutline, "DepthOutline", "PostEffect/DepthOutline.PS", kDepthOutlineDefaults, kDepthOutlineParameters),
	CreateDefinition(PostEffectType::LensDistortion, "LensDistortion", "PostEffect/LensDistortion.PS", kLensDistortionDefaults, kLensDistortionParameters),
	CreateDefinition(PostEffectType::LuminanceOutline, "LuminanceOutline", "PostEffect/LuminanceOutline.PS", kLuminanceOutlineDefaults, kLuminanceOutlineParameters),
	CreateDefinition(PostEffectType::PixelArt, "PixelArt", "PostEffect/PixelArt.PS", kPixelArtDefaults, kPixelArtParameters),
	CreateDefinition(PostEffectType::RadialBlur, "RadialBlur", "PostEffect/RadialBlur.PS", kRadialBlurDefaults, kRadialBlurParameters),
	CreateDefinition(PostEffectType::Random, "Random", "PostEffect/Random.PS", kRandomDefaults, kRandomParameters),
	CreateParameterlessDefinition(PostEffectType::Sepia, "Sepia", "PostEffect/Sepia.PS"),
	CreateDefinition(PostEffectType::SplitToning, "SplitToning", "PostEffect/SplitToning.PS", kSplitToningDefaults, kSplitToningParameters),
	CreateDefinition(PostEffectType::Toon, "Toon", "PostEffect/Toon.PS", kToonDefaults, kToonParameters),
	CreateDefinition(PostEffectType::Vignette, "Vignette", "PostEffect/Vignette.PS", kVignetteDefaults, kVignetteParameters),
	CreateDefinition(PostEffectType::FXAA, "FXAA", "PostEffect/FXAA.PS", kFXAADefaults, kFXAAParameters),
	CreateDefinition(PostEffectType::ToneMapping, "ToneMapping", "PostEffect/ToneMapping.PS", kToneMappingDefaults, kToneMappingParameters),
};

} // namespace

const PostEffectDefinition* PostEffectDefinitionRegistry::Find(PostEffectType type) {
	for (const PostEffectDefinition& definition : kDefinitions) {
		if (definition.type == type) {
			return &definition;
		}
	}

	return nullptr;
}

const PostEffectDefinition* PostEffectDefinitionRegistry::FindByShaderKey(std::string_view shaderKey) {
	for (const PostEffectDefinition& definition : kDefinitions) {
		if (definition.shaderKey == shaderKey) {
			return &definition;
		}
	}

	return nullptr;
}

const PostEffectDefinition* PostEffectDefinitionRegistry::FindByTypeName(std::string_view typeName) {
	for (const PostEffectDefinition& definition : kDefinitions) {
		if (definition.typeName == typeName) {
			return &definition;
		}
	}

	return nullptr;
}

std::span<const PostEffectDefinition> PostEffectDefinitionRegistry::GetAll() {
	return kDefinitions;
}

} // namespace MadoEngine::Render
