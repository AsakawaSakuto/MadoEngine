#include "PostEffectEditor.h"
#include "Utility/Json/JsonHeaders.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>

namespace MadoEngine::Editor {

    namespace {
        /// @brief 固定配列の要素数を取得する
        /// @tparam T 配列要素の型
        /// @tparam N 配列要素数
        /// @param values 要素数を取得する配列
        /// @return 配列要素数
        template<typename T, std::size_t N>
        constexpr std::size_t CountOf(const T(&values)[N]) {
            (void)values;
            return N;
        }

        /// @brief PostEffect定義内のfloatパラメータ情報
        struct PostEffectFloatParameterDefinition {
            const char* key;
            const char* label;
            std::size_t offset;
            float minValue;
            float maxValue;
            float speed;
        };

        /// @brief ColorEditでまとめて扱う色パラメータ情報
        struct PostEffectColorParameterGroup {
            const Render::LayerEffectPass::FloatParameterControl* controls[4] = {};
            std::string key;
            std::string label;
            int componentCount = 0;
        };

        /// @brief ImGuiから選択可能なPostEffect定義
        struct PostEffectDefinition {
            const char* displayName;
            const char* shaderKey;
            const float* initialValues;
            std::size_t initialValueCount;
            const PostEffectFloatParameterDefinition* parameters;
            std::size_t parameterCount;
        };

        /// @brief Passの所属配列種別
        enum class LayerEffectPassListType {
            Layer,
            Screen,
        };

        /// @brief 削除ボタンが押されたPass情報
        struct LayerEffectPassRemoveRequest {
            bool isRequested = false;
            LayerEffectPassListType listType = LayerEffectPassListType::Layer;
            std::size_t index = 0;
        };

        constexpr std::size_t kFloatSize = sizeof(float);

        const float kBloomInitialValues[] = {
            0.6f, 0.7f, 4.0f, 0.5f,
            1.0f, 1.0f, 1.0f, 1.0f
        };
        const PostEffectFloatParameterDefinition kBloomParameters[] = {
            { "Intensity", "強度", 0 * kFloatSize, 0.0f, 5.0f, 0.01f },
            { "Threshold", "しきい値", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Radius", "半径", 2 * kFloatSize, 0.0f, 32.0f, 0.1f },
            { "SoftKnee", "ソフトニー", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "BloomColorR", "発光色R", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "BloomColorG", "発光色G", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "BloomColorB", "発光色B", 6 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "BloomColorA", "発光色A", 7 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kBinarizeInitialValues[] = {
            0.5f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kBinarizeParameters[] = {
            { "Threshold", "しきい値", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Intensity", "適用率", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "LowColorR", "低輝度色R", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "LowColorG", "低輝度色G", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "LowColorB", "低輝度色B", 6 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "HighColorR", "高輝度色R", 8 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "HighColorG", "高輝度色G", 9 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "HighColorB", "高輝度色B", 10 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kChromaticAberrationInitialValues[] = {
            3.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kChromaticAberrationParameters[] = {
            { "OffsetPixels", "ずれ量", 0 * kFloatSize, 0.0f, 32.0f, 0.1f },
            { "EdgeStrength", "外周強度", 1 * kFloatSize, 0.001f, 4.0f, 0.01f },
            { "Intensity", "適用率", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "CenterX", "中心X", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "CenterY", "中心Y", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kColorFilterInitialValues[] = {
            1.0f, 0.85f, 0.65f, 1.0f
        };
        const PostEffectFloatParameterDefinition kColorFilterParameters[] = {
            { "FilterColorR", "フィルター色R", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "FilterColorG", "フィルター色G", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "FilterColorB", "フィルター色B", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Intensity", "適用率", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kDepthOfFieldInitialValues[] = {
            300.0f, 120.0f, 8.0f, 1.0f,
            0.1f, 1000.0f, 1.0f, 1.0f
        };
        const PostEffectFloatParameterDefinition kDepthOfFieldParameters[] = {
            { "FocusDistance", "焦点距離", 0 * kFloatSize, 0.0f, 5000.0f, 1.0f },
            { "FocusRange", "焦点幅", 1 * kFloatSize, 0.001f, 5000.0f, 1.0f },
            { "BlurRadius", "ぼかし半径", 2 * kFloatSize, 0.0f, 32.0f, 0.1f },
            { "Intensity", "適用率", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "NearClip", "NearClip", 4 * kFloatSize, 0.001f, 100.0f, 0.01f },
            { "FarClip", "FarClip", 5 * kFloatSize, 1.0f, 10000.0f, 1.0f },
            { "ForegroundStrength", "手前ぼけ強度", 6 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "BackgroundStrength", "奥ぼけ強度", 7 * kFloatSize, 0.0f, 4.0f, 0.01f },
        };

        const float kLensDistortionInitialValues[] = {
            0.18f, 0.04f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kLensDistortionParameters[] = {
            { "Distortion", "歪み量", 0 * kFloatSize, -1.0f, 1.0f, 0.001f },
            { "CubicDistortion", "二次歪み量", 1 * kFloatSize, -1.0f, 1.0f, 0.001f },
            { "Zoom", "ズーム", 2 * kFloatSize, 0.25f, 4.0f, 0.01f },
            { "Intensity", "適用率", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "CenterX", "中心X", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "CenterY", "中心Y", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kBoxFilterInitialValues[] = {
            1.0f, 1.0f, 0.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kBoxFilterParameters[] = {
            { "Radius", "半径", 0 * kFloatSize, 1.0f, 8.0f, 1.0f },
            { "Intensity", "適用率", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kGaussianFilterInitialValues[] = {
            1.6f, 2.0f, 1.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kGaussianFilterParameters[] = {
            { "Sigma", "標準偏差", 0 * kFloatSize, 0.001f, 8.0f, 0.01f },
            { "Radius", "半径", 1 * kFloatSize, 1.0f, 8.0f, 1.0f },
            { "Intensity", "適用率", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kVignetteInitialValues[] = {
            0.8f, 0.35f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        const PostEffectFloatParameterDefinition kVignetteParameters[] = {
            { "Intensity", "強度", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "InnerRadius", "内側半径", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "OuterScale", "外側倍率", 2 * kFloatSize, 1.0f, 4.0f, 0.01f },
            { "ColorR", "色R", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorG", "色G", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorB", "色B", 6 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorA", "色A", 7 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kPixelArtInitialValues[] = {
            6.0f, 8.0f, 1.15f, 1.0f
        };
        const PostEffectFloatParameterDefinition kPixelArtParameters[] = {
            { "PixelSize", "ピクセルサイズ", 0 * kFloatSize, 1.0f, 64.0f, 1.0f },
            { "ColorSteps", "色階調数", 1 * kFloatSize, 2.0f, 32.0f, 1.0f },
            { "Contrast", "コントラスト", 2 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "Intensity", "適用率", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kToonInitialValues[] = {
            4.0f, 1.15f, 1.1f, 1.0f,
            1.25f, 80.0f, 0.005f, 1.0f,
            0.02f, 0.025f, 0.03f, 1.0f
        };
        const PostEffectFloatParameterDefinition kToonParameters[] = {
            { "ColorSteps", "色階調数", 0 * kFloatSize, 2.0f, 12.0f, 1.0f },
            { "Saturation", "彩度", 1 * kFloatSize, 0.0f, 3.0f, 0.01f },
            { "Contrast", "コントラスト", 2 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "Intensity", "適用率", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Thickness", "輪郭太さ", 4 * kFloatSize, 0.25f, 12.0f, 0.05f },
            { "DepthSensitivity", "深度感度", 5 * kFloatSize, 1.0f, 300.0f, 1.0f },
            { "EdgeThreshold", "輪郭しきい値", 6 * kFloatSize, 0.0001f, 0.1f, 0.0001f },
            { "EdgeIntensity", "輪郭強度", 7 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "OutlineColorR", "輪郭色R", 8 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "OutlineColorG", "輪郭色G", 9 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "OutlineColorB", "輪郭色B", 10 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kOutlineInitialValues[] = {
            1.0f, 0.85f, 0.15f, 1.0f,
            1.0f, 80.0f, 0.005f, 1.0f
        };
        const PostEffectFloatParameterDefinition kOutlineParameters[] = {
            { "ColorR", "色R", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorG", "色G", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorB", "色B", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorA", "色A", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Thickness", "太さ", 4 * kFloatSize, 0.25f, 12.0f, 0.05f },
            { "DepthSensitivity", "深度感度", 5 * kFloatSize, 1.0f, 300.0f, 1.0f },
            { "EdgeThreshold", "エッジしきい値", 6 * kFloatSize, 0.0001f, 0.1f, 0.0001f },
            { "Intensity", "濃さ", 7 * kFloatSize, 0.0f, 4.0f, 0.01f },
        };

        const float kLuminanceBasedOutlineInitialValues[] = {
            0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 4.0f, 0.05f, 1.0f
        };
        const PostEffectFloatParameterDefinition kLuminanceBasedOutlineParameters[] = {
            { "ColorR", "輪郭色R", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorG", "輪郭色G", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorB", "輪郭色B", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorA", "輪郭色A", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Thickness", "太さ", 4 * kFloatSize, 0.25f, 12.0f, 0.05f },
            { "LuminanceSensitivity", "輝度感度", 5 * kFloatSize, 0.1f, 32.0f, 0.01f },
            { "EdgeThreshold", "エッジしきい値", 6 * kFloatSize, 0.0001f, 0.5f, 0.0001f },
            { "Intensity", "強さ", 7 * kFloatSize, 0.0f, 4.0f, 0.01f },
        };

        const float kFogInitialValues[] = {
            0.58f, 0.68f, 0.74f, 1.0f,
            850.0f, 1000.0f, 1.0f, 0.0f,
            0.1f, 1000.0f, 0.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kFogParameters[] = {
            { "ColorR", "色R", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorG", "色G", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorB", "色B", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorA", "色A", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "StartDistance", "開始距離", 4 * kFloatSize, 0.0f, 5000.0f, 1.0f },
            { "EndDistance", "終了距離", 5 * kFloatSize, 0.0f, 5000.0f, 1.0f },
            { "Density", "濃度", 6 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "HeightStrength", "高さ強度", 7 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "NearClip", "NearClip", 8 * kFloatSize, 0.001f, 100.0f, 0.01f },
            { "FarClip", "FarClip", 9 * kFloatSize, 1.0f, 10000.0f, 1.0f },
        };

        const float kDissolveInitialValues[] = {
            0.35f, 0.06f, 1.0f, 2.0f,
            1.0f, 0.45f, 0.05f, 1.0f
        };
        const PostEffectFloatParameterDefinition kDissolveParameters[] = {
            { "Amount", "進行度", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeWidth", "境界幅", 1 * kFloatSize, 0.001f, 0.5f, 0.001f },
            { "EdgeIntensity", "境界強度", 2 * kFloatSize, 0.0f, 8.0f, 0.01f },
            { "NoiseScale", "ノイズ倍率", 3 * kFloatSize, 0.001f, 32.0f, 0.01f },
            { "EdgeColorR", "境界色R", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeColorG", "境界色G", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeColorB", "境界色B", 6 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeColorA", "境界色A", 7 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kRadialBlurInitialValues[] = {
            0.45f, 16.0f, 0.45f, 1.0f,
            0.5f, 0.5f, 0.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kRadialBlurParameters[] = {
            { "Intensity", "強度", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "SampleCount", "サンプル数", 1 * kFloatSize, 1.0f, 64.0f, 1.0f },
            { "Radius", "半径", 2 * kFloatSize, 0.0f, 2.0f, 0.01f },
            { "Falloff", "距離減衰", 3 * kFloatSize, 0.1f, 4.0f, 0.01f },
            { "CenterX", "中心X", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "CenterY", "中心Y", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kRandomInitialValues[] = {
            1.0f, 1.0f, 1.0f, 1.0f
        };
        const PostEffectFloatParameterDefinition kRandomParameters[] = {
            { "Time", "時間", 0 * kFloatSize, 0.0001f, 1000.0f, 0.01f },
            { "NoiseScale", "ノイズ拡大率", 1 * kFloatSize, 0.0001f, 256.0f, 0.1f },
            { "Contrast", "コントラスト", 2 * kFloatSize, 0.0f, 8.0f, 0.01f },
            { "Intensity", "適用率", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kSplitToningInitialValues[] = {
            0.12f, 0.25f, 0.75f, 0.45f,
            1.0f, 0.72f, 0.35f, 0.35f,
            0.0f, 0.2f, 1.0f, 0.75f
        };
        const PostEffectFloatParameterDefinition kSplitToningParameters[] = {
            { "ShadowColorR", "暗部色R", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ShadowColorG", "暗部色G", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ShadowColorB", "暗部色B", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ShadowAmount", "暗部適用量", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "HighlightColorR", "明部色R", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "HighlightColorG", "明部色G", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "HighlightColorB", "明部色B", 6 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "HighlightAmount", "明部適用量", 7 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Balance", "分岐位置", 8 * kFloatSize, -1.0f, 1.0f, 0.01f },
            { "Softness", "なじみ幅", 9 * kFloatSize, 0.001f, 1.0f, 0.001f },
            { "Intensity", "適用率", 10 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "PreserveLuminance", "輝度保持率", 11 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const PostEffectDefinition kPostEffectDefinitions[] = {
            { "CopyImage", "PostEffect/CopyImage.PS", nullptr, 0, nullptr, 0 },
            { "Binarize", "PostEffect/Binarize.PS", kBinarizeInitialValues, CountOf(kBinarizeInitialValues), kBinarizeParameters, CountOf(kBinarizeParameters) },
            { "Bloom", "PostEffect/Bloom.PS", kBloomInitialValues, CountOf(kBloomInitialValues), kBloomParameters, CountOf(kBloomParameters) },
            { "BoxFilter", "PostEffect/BoxFilter.PS", kBoxFilterInitialValues, CountOf(kBoxFilterInitialValues), kBoxFilterParameters, CountOf(kBoxFilterParameters) },
            { "ChromaticAberration", "PostEffect/ChromaticAberration.PS", kChromaticAberrationInitialValues, CountOf(kChromaticAberrationInitialValues), kChromaticAberrationParameters, CountOf(kChromaticAberrationParameters) },
            { "ColorFilter", "PostEffect/ColorFilter.PS", kColorFilterInitialValues, CountOf(kColorFilterInitialValues), kColorFilterParameters, CountOf(kColorFilterParameters) },
            { "DepthOfField", "PostEffect/DepthOfField.PS", kDepthOfFieldInitialValues, CountOf(kDepthOfFieldInitialValues), kDepthOfFieldParameters, CountOf(kDepthOfFieldParameters) },
            { "Dissolve", "PostEffect/Dissolve.PS", kDissolveInitialValues, CountOf(kDissolveInitialValues), kDissolveParameters, CountOf(kDissolveParameters) },
            { "Fog", "PostEffect/Fog.PS", kFogInitialValues, CountOf(kFogInitialValues), kFogParameters, CountOf(kFogParameters) },
            { "GaussianFilter", "PostEffect/GaussianFilter.PS", kGaussianFilterInitialValues, CountOf(kGaussianFilterInitialValues), kGaussianFilterParameters, CountOf(kGaussianFilterParameters) },
            { "GrayScale", "PostEffect/GrayScale.PS", nullptr, 0, nullptr, 0 },
            { "Invert", "PostEffect/Invert.PS", nullptr, 0, nullptr, 0 },
            { "DepthOutline", "PostEffect/DepthOutline.PS", kOutlineInitialValues, CountOf(kOutlineInitialValues), kOutlineParameters, CountOf(kOutlineParameters) },
            { "LensDistortion", "PostEffect/LensDistortion.PS", kLensDistortionInitialValues, CountOf(kLensDistortionInitialValues), kLensDistortionParameters, CountOf(kLensDistortionParameters) },
            { "LuminanceOutline", "PostEffect/LuminanceOutline.PS", kLuminanceBasedOutlineInitialValues, CountOf(kLuminanceBasedOutlineInitialValues), kLuminanceBasedOutlineParameters, CountOf(kLuminanceBasedOutlineParameters) },
            { "PixelArt", "PostEffect/PixelArt.PS", kPixelArtInitialValues, CountOf(kPixelArtInitialValues), kPixelArtParameters, CountOf(kPixelArtParameters) },
            { "RadialBlur", "PostEffect/RadialBlur.PS", kRadialBlurInitialValues, CountOf(kRadialBlurInitialValues), kRadialBlurParameters, CountOf(kRadialBlurParameters) },
            { "Random", "PostEffect/Random.PS", kRandomInitialValues, CountOf(kRandomInitialValues), kRandomParameters, CountOf(kRandomParameters) },
            { "Sepia", "PostEffect/Sepia.PS", nullptr, 0, nullptr, 0 },
            { "SplitToning", "PostEffect/SplitToning.PS", kSplitToningInitialValues, CountOf(kSplitToningInitialValues), kSplitToningParameters, CountOf(kSplitToningParameters) },
            { "Toon", "PostEffect/Toon.PS", kToonInitialValues, CountOf(kToonInitialValues), kToonParameters, CountOf(kToonParameters) },
            { "Vignette", "PostEffect/Vignette.PS", kVignetteInitialValues, CountOf(kVignetteInitialValues), kVignetteParameters, CountOf(kVignetteParameters) },
        };

        const std::filesystem::path kLayerEffectPassEditorJsonPath = "Assets/Json/LayerEffectPassEditor.json";

        /// @brief 指定したJsonパスのバックアップパスを作成する。
        /// @param filePath バックアップ元のJsonパス。
        /// @return .bakを付けたバックアップJsonパス。
        std::filesystem::path CreateBackupJsonPath(const std::filesystem::path& filePath) {
            std::filesystem::path backupPath = filePath;
            backupPath += ".bak";
            return backupPath;
        }

        /// @brief shaderKeyに一致するPostEffect定義を取得する。
        /// @param shaderKey 検索するPixelShaderキー。
        /// @return 一致したPostEffect定義。一致しない場合はnullptr。
        const PostEffectDefinition* FindPostEffectDefinition(const std::string& shaderKey) {
            for (const PostEffectDefinition& definition : kPostEffectDefinitions) {
                if (shaderKey == definition.shaderKey) {
                    return &definition;
                }
            }

            return nullptr;
        }

        /// @brief Passのfloatパラメータ値をJsonへ変換する。
        /// @param pass 保存対象のPass。
        /// @return パラメータ名と値を保持したJson。
        nlohmann::json SerializeLayerEffectPassParameters(const Render::LayerEffectPass& pass) {
            nlohmann::json parameters = nlohmann::json::object();
            for (const Render::LayerEffectPass::FloatParameterControl& control : pass.GetFloatParameterControls()) {
                float value = 0.0f;
                if (pass.TryGetFloatParameter(control.offset, value)) {
                    parameters[control.key] = value;
                }
            }

            return parameters;
        }

        /// @brief PassのEditor設定をJsonへ変換する。
        /// @param pass 保存対象のPass。
        /// @return Pass設定を保持したJson。
        nlohmann::json SerializeLayerEffectPass(const Render::LayerEffectPass& pass) {
            nlohmann::json passJson;
            passJson["key"] = pass.GetKey();
            passJson["name"] = pass.GetName();
            passJson["enabled"] = pass.IsEnabled();
            passJson["targetLayerMask"] = pass.GetTargetLayerMask();
            passJson["effectShaderKey"] = pass.GetEffectShaderKey();
            passJson["ignoreDepthForMask"] = pass.IsIgnoreDepthForMask();
            passJson["parameters"] = SerializeLayerEffectPassParameters(pass);
            return passJson;
        }

        /// @brief Pass配列をJsonへ変換する。
        /// @param passes 保存対象のPass配列。
        /// @return Pass配列を保持したJson。
        nlohmann::json SerializeLayerEffectPassList(const std::vector<Render::LayerEffectPass>& passes) {
            nlohmann::json passList = nlohmann::json::array();
            for (const Render::LayerEffectPass& pass : passes) {
                passList.push_back(SerializeLayerEffectPass(pass));
            }

            return passList;
        }

        /// @brief Layer Effect Pass Editorの状態をJsonへ保存する。
        /// @param postEffectManager 保存対象のPostEffectManager。
        /// @return 保存に成功した場合はtrue。
        bool SaveLayerEffectPassEditorJson(const Render::PostEffectManager& postEffectManager) {
            nlohmann::json root;
            root["version"] = 1;
            root["layerPasses"] = SerializeLayerEffectPassList(postEffectManager.GetLayerPasses());
            root["screenPasses"] = SerializeLayerEffectPassList(postEffectManager.GetScreenPasses());

            return Json::JsonFile::Save(kLayerEffectPassEditorJsonPath, root, 4, true);
        }

        /// @brief shaderKeyに一致するPostEffect定義のindexを取得する。
        /// @param shaderKey 検索するPixelShaderキー。
        /// @return 一致した定義index。一致しない場合は0。
        int FindPostEffectDefinitionIndex(const std::string& shaderKey) {
            for (int index = 0; index < static_cast<int>(CountOf(kPostEffectDefinitions)); ++index) {
                if (shaderKey == kPostEffectDefinitions[index].shaderKey) {
                    return index;
                }
            }

            return 0;
        }

        /// @brief PostEffect定義をPassへ反映する
        /// @param pass 反映先のPass
        /// @param definition 反映するPostEffect定義
        void ApplyPostEffectDefinition(Render::LayerEffectPass& pass, const PostEffectDefinition& definition) {
            pass.SetEffectShaderKey(definition.shaderKey);
            pass.ClearFloatParameterControls();
            pass.ClearParameterData();

            if (definition.initialValues && definition.initialValueCount > 0) {
                pass.SetParameterData(definition.initialValues, definition.initialValueCount * sizeof(float));

                for (std::size_t i = 0; i < definition.parameterCount; ++i) {
                    const PostEffectFloatParameterDefinition& parameter = definition.parameters[i];
                    pass.AddFloatParameterControl(
                        parameter.key,
                        parameter.label,
                        parameter.offset,
                        parameter.minValue,
                        parameter.maxValue,
                        parameter.speed
                    );
                }
            }
        }

        /// @brief Jsonから読み込んだパラメータ値をPassへ適用する。
        /// @param pass 適用先のPass。
        /// @param parameters パラメータ値を保持したJson。
        void ApplyLayerEffectPassParameters(Render::LayerEffectPass& pass, const nlohmann::json& parameters) {
            if (!parameters.is_object()) {
                return;
            }

            for (const Render::LayerEffectPass::FloatParameterControl& control : pass.GetFloatParameterControls()) {
                const auto parameterIt = parameters.find(control.key);
                if (parameterIt == parameters.end() || !parameterIt->is_number()) {
                    continue;
                }

                pass.SetFloatParameter(control.offset, parameterIt->get<float>());
            }
        }

        /// @brief Jsonから読み込んだPass設定を生成用Descへ変換する。
        /// @param passJson Pass設定を保持したJson。
        /// @param defaultKey keyが未指定だった場合に使用する値。
        /// @param defaultName nameが未指定だった場合に使用する値。
        /// @param defaultLayerMask targetLayerMaskが未指定だった場合に使用する値。
        /// @return LayerEffectPass生成用Desc。
        Render::LayerEffectPass::Desc CreateLayerEffectPassDescFromJson(
            const nlohmann::json& passJson,
            const std::string& defaultKey,
            const std::string& defaultName,
            Render::RenderLayerMask defaultLayerMask)
        {
            Render::LayerEffectPass::Desc desc{};
            desc.key = passJson.value("key", defaultKey);
            desc.name = passJson.value("name", defaultName);
            desc.targetLayerMask = passJson.value("targetLayerMask", defaultLayerMask);
            desc.effectShaderKey = passJson.value("effectShaderKey", std::string("PostEffect/CopyImage.PS"));
            desc.enabled = passJson.value("enabled", true);
            desc.ignoreDepthForMask = passJson.value("ignoreDepthForMask", false);

            if (desc.key.empty()) {
                desc.key = defaultKey;
            }
            if (desc.name.empty()) {
                desc.name = defaultName;
            }
            if (desc.effectShaderKey.empty()) {
                desc.effectShaderKey = "PostEffect/CopyImage.PS";
            }

            return desc;
        }

        /// @brief Jsonから読み込んだPass設定をPostEffectManagerへ追加する。
        /// @param postEffectManager 追加先のPostEffectManager。
        /// @param passJson Pass設定を保持したJson。
        /// @param isScreenPass フルスクリーンPassとして追加する場合はtrue。
        /// @param index デフォルト名に使用するPass番号。
        void AddLayerEffectPassFromJson(
            Render::PostEffectManager& postEffectManager,
            const nlohmann::json& passJson,
            bool isScreenPass,
            std::size_t index)
        {
            if (!passJson.is_object()) {
                return;
            }

            const std::string defaultKey =
                (isScreenPass ? "ScreenEffectPass_Loaded_" : "LayerEffectPass_Loaded_") + std::to_string(index + 1);
            const std::string defaultName =
                (isScreenPass ? "Screen Effect Loaded " : "Layer Effect Loaded ") + std::to_string(index + 1);
            const Render::RenderLayerMask defaultLayerMask = isScreenPass ?
                Render::kAllRenderLayers :
                Render::ToRenderLayerMask(Render::RenderLayer::Default);
            const Render::LayerEffectPass::Desc desc =
                CreateLayerEffectPassDescFromJson(passJson, defaultKey, defaultName, defaultLayerMask);

            Render::LayerEffectPass* pass = isScreenPass ?
                postEffectManager.AddScreenPass(desc) :
                postEffectManager.AddLayerPass(desc);
            if (!pass) {
                return;
            }

            if (const PostEffectDefinition* definition = FindPostEffectDefinition(desc.effectShaderKey)) {
                ApplyPostEffectDefinition(*pass, *definition);
            }

            const auto parametersIt = passJson.find("parameters");
            if (parametersIt != passJson.end()) {
                ApplyLayerEffectPassParameters(*pass, *parametersIt);
            }
        }

        /// @brief 既存Passを破棄してJson読込用に空の状態へ戻す。
        /// @param postEffectManager 初期化対象のPostEffectManager。
        void ClearLayerEffectPassesForLoad(Render::PostEffectManager& postEffectManager) {
            for (Render::LayerEffectPass& pass : postEffectManager.GetLayerPasses()) {
                pass.ClearParameterData();
            }

            for (Render::LayerEffectPass& pass : postEffectManager.GetScreenPasses()) {
                pass.ClearParameterData();
            }

            postEffectManager.ClearLayerPasses();
            postEffectManager.ClearScreenPasses();
        }

        /// @brief JsonのPass配列をPostEffectManagerへ読み込む。
        /// @param postEffectManager 読み込み先のPostEffectManager。
        /// @param passList Pass配列を保持したJson。
        /// @param isScreenPass フルスクリーンPassとして読み込む場合はtrue。
        void LoadLayerEffectPassListFromJson(
            Render::PostEffectManager& postEffectManager,
            const nlohmann::json& passList,
            bool isScreenPass)
        {
            if (!passList.is_array()) {
                return;
            }

            for (std::size_t i = 0; i < passList.size(); ++i) {
                AddLayerEffectPassFromJson(postEffectManager, passList[i], isScreenPass, i);
            }
        }

        /// @brief Layer Effect Pass Editorの状態をJsonから読み込む。
        /// @param postEffectManager 読み込み先のPostEffectManager。
        /// @return 読み込みに成功した場合はtrue。
        bool LoadLayerEffectPassEditorJsonInternal(
            Render::PostEffectManager& postEffectManager,
            const std::filesystem::path& filePath = kLayerEffectPassEditorJsonPath)
        {
            nlohmann::json root;
            if (!Json::JsonFile::Load(filePath, root)) {
                return false;
            }

            ClearLayerEffectPassesForLoad(postEffectManager);
            LoadLayerEffectPassListFromJson(postEffectManager, root.value("layerPasses", nlohmann::json::array()), false);
            LoadLayerEffectPassListFromJson(postEffectManager, root.value("screenPasses", nlohmann::json::array()), true);
            return true;
        }

#ifdef USE_IMGUI

        /// @brief 重複しないPassキーを作成する。
        /// @param postEffectManager 重複確認に使用する管理クラス。
        /// @param prefix Passキーの接頭辞。
        /// @param nextId 次に試すID。
        /// @return 重複しないPassキー。
        std::string CreateUniquePassKey(
            const Render::PostEffectManager& postEffectManager,
            const char* prefix,
            int& nextId)
        {
            std::string key;
            do {
                key = std::string(prefix) + std::to_string(nextId);
                ++nextId;
            } while (postEffectManager.FindPass(key));

            return key;
        }

        /// @brief Layer Effect Passを追加する
        /// @param postEffectManager 追加先のポストエフェクト管理クラス
        /// @param isScreenPass フルスクリーンPassを追加する場合はtrue
        void AddLayerEffectPassFromEditor(Render::PostEffectManager& postEffectManager, bool isScreenPass) {
            static int nextLayerPassId = 1;
            static int nextScreenPassId = 1;

            const PostEffectDefinition& definition = kPostEffectDefinitions[0];
            Render::LayerEffectPass::Desc desc{};
            if (isScreenPass) {
                desc.key = CreateUniquePassKey(postEffectManager, "ScreenEffectPass_", nextScreenPassId);
                desc.name = "Screen Effect " + std::to_string(nextScreenPassId - 1);
                desc.targetLayerMask = Render::kAllRenderLayers;
            } else {
                desc.key = CreateUniquePassKey(postEffectManager, "LayerEffectPass_", nextLayerPassId);
                desc.name = "Layer Effect " + std::to_string(nextLayerPassId - 1);
                desc.targetLayerMask = Render::ToRenderLayerMask(Render::RenderLayer::Default);
            }
            desc.effectShaderKey = definition.shaderKey;
            desc.enabled = true;
            desc.ignoreDepthForMask = false;

            Render::LayerEffectPass* pass = isScreenPass ?
                postEffectManager.AddScreenPass(desc) :
                postEffectManager.AddLayerPass(desc);
            if (pass) {
                ApplyPostEffectDefinition(*pass, definition);
            }
        }

        /// @brief 対象Layerの選択Comboを描画する
        /// @param pass 編集対象のPass
        void DrawLayerSelectionCombo(Render::LayerEffectPass& pass) {
            const Render::RenderLayerMask currentLayerMask = pass.GetTargetLayerMask();
            const char* previewName = Render::GetRenderLayerMaskName(currentLayerMask);

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##TargetLayer", previewName)) {
                for (uint32_t index = 0; index < Render::kRenderLayerCount; ++index) {
                    const Render::RenderLayer layer = Render::GetRenderLayerByIndex(index);
                    const Render::RenderLayerMask layerMask = Render::ToRenderLayerMask(layer);
                    const bool isSelected = layerMask == currentLayerMask;
                    if (ImGui::Selectable(Render::GetRenderLayerName(layer), isSelected)) {
                        pass.SetTargetLayerMask(layerMask);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                const bool isAllSelected = Render::kAllRenderLayers == currentLayerMask;
                if (ImGui::Selectable("All", isAllSelected)) {
                    pass.SetTargetLayerMask(Render::kAllRenderLayers);
                }

                if (isAllSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
        }

        /// @brief PostEffect選択Comboを描画する
        /// @param pass 編集対象のPass
        void DrawPostEffectSelectionCombo(Render::LayerEffectPass& pass) {
            const int currentIndex = FindPostEffectDefinitionIndex(pass.GetEffectShaderKey());
            const char* previewName = kPostEffectDefinitions[currentIndex].displayName;

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##PostEffect", previewName)) {
                for (int index = 0; index < static_cast<int>(CountOf(kPostEffectDefinitions)); ++index) {
                    const bool isSelected = index == currentIndex;
                    if (ImGui::Selectable(kPostEffectDefinitions[index].displayName, isSelected)) {
                        ApplyPostEffectDefinition(pass, kPostEffectDefinitions[index]);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        /// @brief 文字列が指定した文字で終わるか確認する
        /// @param text 確認する文字列
        /// @param suffix 末尾に期待する文字
        /// @return 指定した文字で終わる場合はtrue
        bool EndsWith(const std::string& text, char suffix) {
            return !text.empty() && text.back() == suffix;
        }

        /// @brief 色成分キーから共通部分を取得する
        /// @param key 色成分キー
        /// @return R/G/B/Aの末尾を除いたキー
        std::string GetColorBaseKey(const std::string& key) {
            if (EndsWith(key, 'R') || EndsWith(key, 'G') || EndsWith(key, 'B') || EndsWith(key, 'A')) {
                return key.substr(0, key.size() - 1);
            }

            return key;
        }

        /// @brief 色成分ラベルから共通表示名を取得する
        /// @param label 色成分ラベル
        /// @return R/G/B/Aの末尾を除いた表示名
        std::string GetColorBaseLabel(const std::string& label) {
            if (EndsWith(label, 'R') || EndsWith(label, 'G') || EndsWith(label, 'B') || EndsWith(label, 'A')) {
                std::string result = label.substr(0, label.size() - 1);
                while (!result.empty() && result.back() == ' ') {
                    result.pop_back();
                }

                return result.empty() ? label : result;
            }

            return label;
        }

        /// @brief floatパラメータが指定した色成分として扱えるか確認する
        /// @param control 確認するパラメータ
        /// @param baseKey 色パラメータの共通キー
        /// @param component 期待する色成分
        /// @param expectedOffset 期待するConstantBuffer上の位置
        /// @return 色成分として扱える場合はtrue
        bool IsColorComponentControl(
            const Render::LayerEffectPass::FloatParameterControl& control,
            const std::string& baseKey,
            char component,
            std::size_t expectedOffset)
        {
            return control.key == baseKey + component &&
                control.offset == expectedOffset &&
                control.minValue == 0.0f &&
                control.maxValue == 1.0f;
        }

        /// @brief 指定位置からColorEdit用の色パラメータグループを取得する
        /// @param controls floatパラメータ一覧
        /// @param startIndex 確認を開始するindex
        /// @param outGroup 取得した色パラメータグループ
        /// @return 色パラメータとして取得できた場合はtrue
        bool TryGetColorParameterGroup(
            const std::vector<Render::LayerEffectPass::FloatParameterControl>& controls,
            std::size_t startIndex,
            PostEffectColorParameterGroup& outGroup)
        {
            if (startIndex + 2 >= controls.size()) {
                return false;
            }

            const Render::LayerEffectPass::FloatParameterControl& red = controls[startIndex];
            if (!EndsWith(red.key, 'R') || red.minValue != 0.0f || red.maxValue != 1.0f) {
                return false;
            }

            const std::string baseKey = GetColorBaseKey(red.key);
            if (!IsColorComponentControl(controls[startIndex + 1], baseKey, 'G', red.offset + kFloatSize) ||
                !IsColorComponentControl(controls[startIndex + 2], baseKey, 'B', red.offset + kFloatSize * 2)) {
                return false;
            }

            outGroup = {};
            outGroup.controls[0] = &controls[startIndex];
            outGroup.controls[1] = &controls[startIndex + 1];
            outGroup.controls[2] = &controls[startIndex + 2];
            outGroup.key = baseKey;
            outGroup.label = GetColorBaseLabel(red.label);
            outGroup.componentCount = 3;

            if (startIndex + 3 < controls.size() &&
                IsColorComponentControl(controls[startIndex + 3], baseKey, 'A', red.offset + kFloatSize * 3)) {
                outGroup.controls[3] = &controls[startIndex + 3];
                outGroup.componentCount = 4;
            }

            return true;
        }

        /// @brief ColorEditでポストエフェクトの色パラメータを描画する
        /// @param pass 編集対象のPass
        /// @param group 描画する色パラメータグループ
        /// @return 値が変更された場合はtrue
        bool DrawPostEffectColorParameterRow(
            Render::LayerEffectPass& pass,
            const PostEffectColorParameterGroup& group)
        {
            float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            for (int i = 0; i < group.componentCount; ++i) {
                if (!pass.TryGetFloatParameter(group.controls[i]->offset, color[i])) {
                    return false;
                }
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(group.label.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(-1.0f);
            const std::string colorEditId = "##" + group.key;
            bool isChanged = false;
            if (group.componentCount == 4) {
                isChanged = ImGui::ColorEdit4(colorEditId.c_str(), color);
            } else {
                isChanged = ImGui::ColorEdit3(colorEditId.c_str(), color);
            }

            if (isChanged) {
                for (int i = 0; i < group.componentCount; ++i) {
                    pass.SetFloatParameter(group.controls[i]->offset, color[i]);
                }
            }

            return isChanged;
        }

        /// @brief 選択中PostEffectのパラメータ調整行を描画する
        /// @param pass 編集対象のPass
        void DrawPostEffectParameterRows(Render::LayerEffectPass& pass) {
            const std::vector<Render::LayerEffectPass::FloatParameterControl>& controls = pass.GetFloatParameterControls();
            if (controls.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("調整項目はありません");
                return;
            }

            for (std::size_t i = 0; i < controls.size(); ++i) {
                PostEffectColorParameterGroup colorGroup{};
                if (TryGetColorParameterGroup(controls, i, colorGroup)) {
                    DrawPostEffectColorParameterRow(pass, colorGroup);
                    i += static_cast<std::size_t>(colorGroup.componentCount - 1);
                    continue;
                }

                const Render::LayerEffectPass::FloatParameterControl& control = controls[i];
                float value = 0.0f;
                if (!pass.TryGetFloatParameter(control.offset, value)) {
                    continue;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(control.label.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(-1.0f);
                const std::string sliderId = "##" + control.key;
                if (ImGui::DragFloat(sliderId.c_str(), &value, control.speed, control.minValue, control.maxValue)) {
                    pass.SetFloatParameter(control.offset, value);
                }
            }
        }

        /// @brief Layer Effect Pass Editorの1行を描画する
        /// @param pass 編集対象のPass
        /// @param listType Passの所属配列種別
        /// @param index Pass配列内のindex
        /// @param removeRequest 削除要求の出力先
        void DrawLayerEffectPassEditorRow(
            Render::LayerEffectPass& pass,
            LayerEffectPassListType listType,
            std::size_t index,
            LayerEffectPassRemoveRequest& removeRequest)
        {
            ImGui::PushID(pass.GetKey().c_str());

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            bool enabled = pass.IsEnabled();
            if (ImGui::Checkbox("##Enabled", &enabled)) {
                pass.SetEnabled(enabled);
            }

            ImGui::TableNextColumn();
            std::array<char, 128> nameBuffer{};
            std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", pass.GetName().c_str());
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##Name", nameBuffer.data(), nameBuffer.size())) {
                if (nameBuffer[0] != '\0') {
                    pass.SetName(nameBuffer.data());
                }
            }

            ImGui::TableNextColumn();
            DrawPostEffectSelectionCombo(pass);

            ImGui::TableNextColumn();
            if (listType == LayerEffectPassListType::Layer) {
                DrawLayerSelectionCombo(pass);
            } else {
                ImGui::TextDisabled("全画面");
            }

            ImGui::TableNextColumn();
            bool ignoreDepth = pass.IsIgnoreDepthForMask();
            if (listType == LayerEffectPassListType::Layer) {
                if (ImGui::Checkbox("##Depth", &ignoreDepth)) {
                    pass.SetIgnoreDepthForMask(ignoreDepth);
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Checkbox("##Depth", &ignoreDepth);
                ImGui::EndDisabled();
            }

            ImGui::TableNextColumn();
            if (ImGui::Button("削除")) {
                removeRequest.isRequested = true;
                removeRequest.listType = listType;
                removeRequest.index = index;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::TreeNodeEx("値を調整", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns)) {
                DrawPostEffectParameterRows(pass);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }

#endif // USE_IMGUI

    } // namespace

    bool LoadLayerEffectPassEditorJson(Render::PostEffectManager& postEffectManager) {
        return LoadLayerEffectPassEditorJsonInternal(postEffectManager);
    }

    bool LoadLayerEffectPassEditorJsonFromFile(Render::PostEffectManager& postEffectManager) {
        return LoadLayerEffectPassEditorJson(postEffectManager);
    }

#ifdef USE_IMGUI

    void DrawLayerEffectPassEditorUI(Render::PostEffectManager& postEffectManager) {
        static LayerEffectPassListType selectedListType = LayerEffectPassListType::Layer;
        static std::size_t selectedIndex = static_cast<std::size_t>(-1);

        ImGui::Begin("Layer Effect Editor");

        if (ImGui::Button("追加")) {
            AddLayerEffectPassFromEditor(postEffectManager, false);
            selectedListType = LayerEffectPassListType::Layer;
            selectedIndex = postEffectManager.GetLayerPasses().empty() ? static_cast<std::size_t>(-1) : postEffectManager.GetLayerPasses().size() - 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("フルスクリーン追加")) {
            AddLayerEffectPassFromEditor(postEffectManager, true);
            selectedListType = LayerEffectPassListType::Screen;
            selectedIndex = postEffectManager.GetScreenPasses().empty() ? static_cast<std::size_t>(-1) : postEffectManager.GetScreenPasses().size() - 1;
        }

        ImGui::SameLine();
        if (ImGui::Button("保存")) {
            SaveLayerEffectPassEditorJson(postEffectManager);
        }
        ImGui::SameLine();
        if (ImGui::Button("読込")) {
            LoadLayerEffectPassEditorJson(postEffectManager);
        }
        ImGui::SameLine();
        if (ImGui::Button("復元")) {
            LoadLayerEffectPassEditorJsonInternal(postEffectManager, CreateBackupJsonPath(kLayerEffectPassEditorJsonPath));
        }

        ImGui::Separator();

        LayerEffectPassRemoveRequest removeRequest{};

        std::vector<Render::LayerEffectPass>& layerPasses = postEffectManager.GetLayerPasses();
        std::vector<Render::LayerEffectPass>& screenPasses = postEffectManager.GetScreenPasses();

        auto getSelectedPass = [&]() -> Render::LayerEffectPass* {
            if (selectedIndex == static_cast<std::size_t>(-1)) {
                return nullptr;
            }
            if (selectedListType == LayerEffectPassListType::Layer) {
                if (selectedIndex >= layerPasses.size()) {
                    return nullptr;
                }
                return &layerPasses[selectedIndex];
            }
            if (selectedIndex >= screenPasses.size()) {
                return nullptr;
            }
            return &screenPasses[selectedIndex];
        };

        ImGui::BeginChild("LayerEffectPassList", ImVec2(240.0f, 0.0f), true);
        auto drawPassListItem = [&](Render::LayerEffectPass& pass, LayerEffectPassListType listType, std::size_t index) {
            ImGui::PushID(pass.GetKey().c_str());
            const bool selected = selectedListType == listType && selectedIndex == index;
            const char* typeLabel = listType == LayerEffectPassListType::Layer ? "Layer" : "Screen";
            const std::string label = std::string("[") + typeLabel + "] " + pass.GetName();
            const float deleteButtonWidth = ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float selectableWidth = ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x;
            if (selectableWidth < 1.0f) {
                selectableWidth = 1.0f;
            }
            if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(selectableWidth, 0.0f))) {
                selectedListType = listType;
                selectedIndex = index;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("削除")) {
                removeRequest.isRequested = true;
                removeRequest.listType = listType;
                removeRequest.index = index;
            }
            ImGui::PopID();
        };

        for (std::size_t i = 0; i < layerPasses.size(); ++i) {
            drawPassListItem(layerPasses[i], LayerEffectPassListType::Layer, i);
        }
        for (std::size_t i = 0; i < screenPasses.size(); ++i) {
            drawPassListItem(screenPasses[i], LayerEffectPassListType::Screen, i);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("LayerEffectPassProperties", ImVec2(0.0f, 0.0f), true);
        Render::LayerEffectPass* selectedPass = getSelectedPass();
        if (selectedPass) {
            ImGui::PushID(selectedPass->GetKey().c_str());

            bool enabled = selectedPass->IsEnabled();
            if (ImGui::Checkbox("Enabled", &enabled)) {
                selectedPass->SetEnabled(enabled);
            }

            std::array<char, 128> nameBuffer{};
            std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", selectedPass->GetName().c_str());
            if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
                if (nameBuffer[0] != '\0') {
                    selectedPass->SetName(nameBuffer.data());
                }
            }

            ImGui::TextUnformatted("Effect");
            DrawPostEffectSelectionCombo(*selectedPass);

            if (selectedListType == LayerEffectPassListType::Layer) {
                ImGui::TextUnformatted("Layer");
                DrawLayerSelectionCombo(*selectedPass);

                bool ignoreDepth = selectedPass->IsIgnoreDepthForMask();
                if (ImGui::Checkbox("Ignore Depth", &ignoreDepth)) {
                    selectedPass->SetIgnoreDepthForMask(ignoreDepth);
                }
            } else {
                ImGui::TextDisabled("Target: Screen");
            }

            constexpr ImGuiTableFlags propertyTableFlags =
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp;
            if (ImGui::BeginTable("LayerEffectPassPropertyTable", 3, propertyTableFlags)) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 1.0f);
                ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch);
                DrawPostEffectParameterRows(*selectedPass);
                ImGui::EndTable();
            }

            ImGui::PopID();
        } else {
            selectedIndex = static_cast<std::size_t>(-1);
            ImGui::TextDisabled("LayerEffectPassを選択してください。");
        }
        ImGui::EndChild();

        if (removeRequest.isRequested) {
            const bool isSelectedRemoved =
                selectedListType == removeRequest.listType &&
                selectedIndex == removeRequest.index;
            if (removeRequest.listType == LayerEffectPassListType::Layer) {
                postEffectManager.RemoveLayerPass(removeRequest.index);
            } else {
                postEffectManager.RemoveScreenPass(removeRequest.index);
            }
            if (isSelectedRemoved) {
                selectedIndex = static_cast<std::size_t>(-1);
            }
        }

        ImGui::End();
    }

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
