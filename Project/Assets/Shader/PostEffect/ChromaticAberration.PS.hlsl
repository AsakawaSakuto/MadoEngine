#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer ChromaticAberrationParams : register(b0) {
    float4 gChromaticAberrationParams;
    float4 gChromaticAberrationCenterParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief 色収差エフェクトの既定値を使用するかを返す
/// @return 既定値を使用する場合はtrue
bool UsesDefaultChromaticAberrationParams() {
    return all(gChromaticAberrationParams == 0.0f) && all(gChromaticAberrationCenterParams == 0.0f);
}

/// @brief 色収差エフェクトの強度パラメータを取得する
/// @return x: ずれ量, y: 外周強度, z: 適用率
float4 GetChromaticAberrationParams() {
    if (UsesDefaultChromaticAberrationParams()) {
        return float4(3.0f, 1.0f, 1.0f, 0.0f);
    }

    return gChromaticAberrationParams;
}

/// @brief 色収差エフェクトの中心座標を取得する
/// @return 画面UV上の中心座標
float2 GetChromaticAberrationCenter() {
    if (UsesDefaultChromaticAberrationParams()) {
        return float2(0.5f, 0.5f);
    }

    return saturate(gChromaticAberrationCenterParams.xy);
}

/// @brief 中心から外側へ向かう色収差のずれ量を計算する
/// @param texcoord 入力UV
/// @param center 色収差の中心UV
/// @param texelSize 1ピクセル分のUVサイズ
/// @param offsetPixels 最大ずれ量
/// @param edgeStrength 外周に向かう強度
/// @return サンプリングに使用するUVずれ量
float2 CalculateChromaticAberrationOffset(
    float2 texcoord,
    float2 center,
    float2 texelSize,
    float offsetPixels,
    float edgeStrength) {
    float2 direction = texcoord - center;
    float distanceFromCenter = length(direction);

    if (distanceFromCenter <= 0.0001f) {
        return float2(0.0f, 0.0f);
    }

    float edgeWeight = pow(saturate(distanceFromCenter * 2.0f), max(edgeStrength, 0.001f));
    return normalize(direction) * texelSize * max(offsetPixels, 0.0f) * edgeWeight;
}

/// @brief 画面色へ色収差を適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return 色収差適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);
    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));

    float4 params = GetChromaticAberrationParams();
    float offsetPixels = max(params.x, 0.0f);
    float edgeStrength = max(params.y, 0.001f);
    float intensity = saturate(params.z);
    float2 center = GetChromaticAberrationCenter();

    float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    float2 chromaticOffset = CalculateChromaticAberrationOffset(input.texcoord, center, texelSize, offsetPixels, edgeStrength);

    float red = gTexture.Sample(gSampler, saturate(input.texcoord + chromaticOffset)).r;
    float green = baseColor.g;
    float blue = gTexture.Sample(gSampler, saturate(input.texcoord - chromaticOffset)).b;
    float3 aberrationColor = float3(red, green, blue);

    output.color = float4(lerp(baseColor.rgb, aberrationColor, intensity), baseColor.a);
    return output;
}
