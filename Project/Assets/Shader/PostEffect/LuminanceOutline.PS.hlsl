#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);
SamplerState gSamplerPoint : register(s1);

cbuffer LuminanceBasedOutlineParams : register(b0) {
    float4 gLuminanceOutlineColor;
    float4 gLuminanceOutlineParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

static const float kPrewittHorizontalKernel[3][3] = {
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float kPrewittVerticalKernel[3][3] = {
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

/// @brief 輝度ベース輪郭の色を取得する
/// @return 輪郭へ合成する色
float4 GetOutlineColor() {
    if (all(gLuminanceOutlineColor == 0.0f)) {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    return gLuminanceOutlineColor;
}

/// @brief 輪郭の太さを取得する
/// @return サンプリング間隔に掛ける太さ
float GetOutlineThickness() {
    if (gLuminanceOutlineParams.x <= 0.0f) {
        return 1.0f;
    }

    return max(gLuminanceOutlineParams.x, 0.25f);
}

/// @brief 輝度差分の感度を取得する
/// @return 輝度差分へ掛ける感度
float GetLuminanceSensitivity() {
    if (gLuminanceOutlineParams.y <= 0.0f) {
        return 4.0f;
    }

    return gLuminanceOutlineParams.y;
}

/// @brief エッジ検出のしきい値を取得する
/// @return 輪郭として扱い始めるしきい値
float GetEdgeThreshold() {
    if (gLuminanceOutlineParams.z <= 0.0f) {
        return 0.05f;
    }

    return gLuminanceOutlineParams.z;
}

/// @brief 輪郭の強さを取得する
/// @return 輪郭の合成倍率
float GetOutlineIntensity() {
    if (gLuminanceOutlineParams.w <= 0.0f) {
        return 1.0f;
    }

    return gLuminanceOutlineParams.w;
}

/// @brief RGBを輝度へ変換する
/// @param color 輝度へ変換するRGB色
/// @return 0から1の輝度
float CalculateLuminance(float3 color) {
    return dot(color, float3(0.2125f, 0.7154f, 0.0721f));
}

/// @brief Prewittフィルタで輝度差分を計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @param thickness 輪郭の太さ
/// @return 横方向と縦方向の輝度差分
float2 CalculateLuminanceDifference(float2 texcoord, float2 texelSize, float thickness) {
    float2 difference = float2(0.0f, 0.0f);

    [unroll]
    for (int x = 0; x < 3; ++x) {
        [unroll]
        for (int y = 0; y < 3; ++y) {
            float2 offset = float2(x - 1, y - 1) * texelSize * thickness;
            float3 fetchColor = gTexture.Sample(gSamplerPoint, texcoord + offset).rgb;
            float luminance = CalculateLuminance(fetchColor);
            difference.x += luminance * kPrewittHorizontalKernel[x][y];
            difference.y += luminance * kPrewittVerticalKernel[x][y];
        }
    }

    return difference;
}

/// @brief 輝度差分から滑らかな輪郭重みを計算する
/// @param edgeValue 輝度差分の大きさ
/// @param threshold 輪郭として扱い始めるしきい値
/// @return 0から1の輪郭重み
float CalculateSmoothEdgeWeight(float edgeValue, float threshold) {
    float smoothWidth = max(threshold * 2.0f, 0.001f);
    return smoothstep(threshold, threshold + smoothWidth, edgeValue);
}

/// @brief 輝度ベースの輪郭重みを計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @return 0から1の輪郭重み
float CalculateOutlineWeight(float2 texcoord, float2 texelSize) {
    float thickness = GetOutlineThickness();
    float threshold = GetEdgeThreshold();
    float2 luminanceDifference = CalculateLuminanceDifference(texcoord, texelSize, thickness);
    float edgeValue = length(luminanceDifference) * GetLuminanceSensitivity();
    return saturate(CalculateSmoothEdgeWeight(edgeValue, threshold) * GetOutlineIntensity());
}

/// @brief 輝度差分で検出した輪郭を画面色へ合成する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return 輝度ベース輪郭適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);

    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));
    float outlineWeight = CalculateOutlineWeight(input.texcoord, texelSize);
    float4 baseColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    float4 outlineColor = GetOutlineColor();

    output.color.rgb = lerp(baseColor.rgb, outlineColor.rgb, outlineWeight * outlineColor.a);
    output.color.a = baseColor.a;
    return output;
}
