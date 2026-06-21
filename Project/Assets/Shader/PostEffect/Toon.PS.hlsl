#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gSceneDepthTexture : register(t1);
Texture2D<float> gMaskDepthTexture : register(t2);
SamplerState gSamplerLinear : register(s0);
SamplerState gSamplerPoint : register(s1);

cbuffer ToonParams : register(b0) {
    float4 gToonColorParams;
    float4 gToonEdgeParams;
    float4 gToonOutlineColor;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

static const float kPrewittHorizontal[3][3] = {
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float kPrewittVertical[3][3] = {
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

/// @brief トゥーン色調整パラメータを取得する
/// @return x: 色階調数, y: 彩度, z: コントラスト, w: 適用率
float4 GetToonColorParams() {
    if (all(gToonColorParams == 0.0f)) {
        return float4(4.0f, 1.15f, 1.1f, 1.0f);
    }

    return gToonColorParams;
}

/// @brief トゥーン輪郭パラメータを取得する
/// @return x: 輪郭太さ, y: 深度感度, z: 輪郭しきい値, w: 輪郭強度
float4 GetToonEdgeParams() {
    if (all(gToonEdgeParams == 0.0f)) {
        return float4(1.25f, 80.0f, 0.005f, 1.0f);
    }

    return gToonEdgeParams;
}

/// @brief 輪郭色を取得する
/// @return 輪郭色
float4 GetToonOutlineColor() {
    if (all(gToonOutlineColor == 0.0f)) {
        return float4(0.02f, 0.025f, 0.03f, 1.0f);
    }

    return gToonOutlineColor;
}

/// @brief 深度が背景クリア値に近いか判定する
/// @param depth 判定する深度
/// @return 背景に近い場合はtrue
bool IsClearDepth(float depth) {
    return depth >= 0.9999f;
}

/// @brief マスク深度がシーン深度より手前か判定する
/// @param maskDepth マスク側の深度
/// @param sceneDepth シーン側の深度
/// @return マスクが見えている場合はtrue
bool IsMaskVisible(float maskDepth, float sceneDepth) {
    return IsClearDepth(sceneDepth) || maskDepth <= sceneDepth + 0.0002f;
}

/// @brief 色の明るさを取得する
/// @param color 入力色
/// @return 輝度
float GetLuminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

/// @brief 彩度を調整する
/// @param color 入力色
/// @param saturation 彩度
/// @return 彩度調整後の色
float3 ApplySaturation(float3 color, float saturation) {
    float luminance = GetLuminance(color);
    return saturate(lerp(float3(luminance, luminance, luminance), color, max(saturation, 0.0f)));
}

/// @brief コントラストを調整する
/// @param color 入力色
/// @param contrast コントラスト
/// @return コントラスト調整後の色
float3 ApplyContrast(float3 color, float contrast) {
    return saturate((color - 0.5f) * max(contrast, 0.0f) + 0.5f);
}

/// @brief 輝度を段階化してトゥーン調の色へ変換する
/// @param color 入力色
/// @param colorSteps 色階調数
/// @return 段階化後の色
float3 QuantizeByLuminance(float3 color, float colorSteps) {
    float steps = max(floor(colorSteps), 2.0f);
    float maxIndex = steps - 1.0f;
    float luminance = max(GetLuminance(color), 0.0001f);
    float toonLuminance = floor(saturate(luminance) * maxIndex + 0.5f) / maxIndex;
    return saturate(color * (toonLuminance / luminance));
}

/// @brief Prewittフィルタで深度差分を計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @param thickness 輪郭太さ
/// @return 深度差分
float2 CalculateDepthDifference(float2 texcoord, float2 texelSize, float thickness) {
    float2 difference = float2(0.0f, 0.0f);

    [unroll]
    for (int x = 0; x < 3; ++x) {
        [unroll]
        for (int y = 0; y < 3; ++y) {
            float2 offset = float2(x - 1, y - 1) * texelSize * thickness;
            float depth = gMaskDepthTexture.Sample(gSamplerPoint, texcoord + offset);
            difference.x += depth * kPrewittHorizontal[x][y];
            difference.y += depth * kPrewittVertical[x][y];
        }
    }

    return difference;
}

/// @brief Prewittフィルタでアルファ差分を計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @param thickness 輪郭太さ
/// @return アルファ差分
float2 CalculateAlphaDifference(float2 texcoord, float2 texelSize, float thickness) {
    float2 difference = float2(0.0f, 0.0f);

    [unroll]
    for (int x = 0; x < 3; ++x) {
        [unroll]
        for (int y = 0; y < 3; ++y) {
            float2 offset = float2(x - 1, y - 1) * texelSize * thickness;
            float alpha = gTexture.Sample(gSamplerPoint, texcoord + offset).a;
            difference.x += alpha * kPrewittHorizontal[x][y];
            difference.y += alpha * kPrewittVertical[x][y];
        }
    }

    return difference;
}

/// @brief 差分から滑らかな輪郭重みを計算する
/// @param edgeValue エッジ差分値
/// @param threshold しきい値
/// @return 0から1の輪郭重み
float CalculateSmoothEdgeWeight(float edgeValue, float threshold) {
    float smoothWidth = max(threshold * 2.0f, 0.001f);
    return smoothstep(threshold, threshold + smoothWidth, edgeValue);
}

/// @brief 深度とアルファから輪郭重みを計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @param edgeParams 輪郭パラメータ
/// @return 輪郭重み
float CalculateOutlineWeight(float2 texcoord, float2 texelSize, float4 edgeParams) {
    float thickness = max(edgeParams.x, 0.25f);
    float depthSensitivity = max(edgeParams.y, 1.0f);
    float threshold = max(edgeParams.z, 0.0001f);
    float intensity = max(edgeParams.w, 0.0f);

    float2 depthDifference = CalculateDepthDifference(texcoord, texelSize, thickness);
    float depthEdge = length(depthDifference) * depthSensitivity;
    float depthWeight = CalculateSmoothEdgeWeight(depthEdge, threshold);

    float2 alphaDifference = CalculateAlphaDifference(texcoord, texelSize, thickness);
    float alphaEdge = length(alphaDifference);
    float alphaWeight = CalculateSmoothEdgeWeight(alphaEdge, 0.02f);

    float centerDepth = gMaskDepthTexture.Sample(gSamplerPoint, texcoord);
    float depthGate = IsClearDepth(centerDepth) ? alphaWeight : saturate(alphaWeight * 2.0f + depthWeight * 0.25f);
    return saturate(max(alphaWeight, depthWeight * depthGate) * intensity);
}

/// @brief 画面色をトゥーン調に変換して輪郭を合成する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return トゥーン適用後の色
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);
    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));

    float4 colorParams = GetToonColorParams();
    float4 edgeParams = GetToonEdgeParams();
    float4 outlineColor = GetToonOutlineColor();

    float4 srcColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    float3 toonColor = QuantizeByLuminance(srcColor.rgb, colorParams.x);
    toonColor = ApplySaturation(toonColor, colorParams.y);
    toonColor = ApplyContrast(toonColor, colorParams.z);
    toonColor = lerp(srcColor.rgb, toonColor, saturate(colorParams.w));

    float outlineWeight = CalculateOutlineWeight(input.texcoord, texelSize, edgeParams);
    float maskDepth = gMaskDepthTexture.Sample(gSamplerPoint, input.texcoord);
    float sceneDepth = gSceneDepthTexture.Sample(gSamplerPoint, input.texcoord);
    float visibleBody = IsMaskVisible(maskDepth, sceneDepth) ? srcColor.a : 0.0f;

    output.color.rgb = lerp(toonColor, outlineColor.rgb, outlineWeight * outlineColor.a);
    output.color.a = max(visibleBody, outlineWeight);
    return output;
}
