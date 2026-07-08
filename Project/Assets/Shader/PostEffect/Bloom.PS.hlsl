#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BloomParams : register(b0)
{
    float4 gBloomParams;
    float4 gBloomColor;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief Bloomの調整パラメータを取得する
/// @return x: 強度, y: しきい値, z: 半径, w: ソフトニー
float4 GetBloomParams() {
    if (all(gBloomParams == 0.0f)) {
        return float4(0.6f, 0.7f, 4.0f, 0.5f);
    }

    return gBloomParams;
}

/// @brief Bloomに乗算する発光色を取得する
/// @return rgb: 発光色, a: 発光色の適用率
float4 GetBloomColor() {
    if (all(gBloomColor == 0.0f)) {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    return saturate(gBloomColor);
}

/// @brief 色の明るさを取得する
/// @param color 入力色
/// @return 色の最大成分
float GetBrightness(float3 color) {
    return max(max(color.r, color.g), color.b);
}

/// @brief Bloom抽出色を取得する
/// @param color 入力色
/// @param params Bloomパラメータ
/// @return しきい値を超えたBloom色
float3 ExtractBloomColor(float3 color, float4 params) {
    float threshold = saturate(params.y);
    float softKnee = saturate(params.w);
    float knee = max(threshold * softKnee, 0.0001f);

    float brightness = GetBrightness(color);
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0f, knee * 2.0f);
    soft = soft * soft / (4.0f * knee);

    float contribution = max(soft, brightness - threshold);
    contribution /= max(brightness, 0.0001f);
    return color * saturate(contribution);
}

/// @brief ずらしたUVからBloom色を取得する
/// @param texcoord サンプリング中心UV
/// @param offset サンプリングオフセット
/// @param params Bloomパラメータ
/// @param bloomColor Bloom発光色
/// @return 発光色を反映したBloom色
float4 SampleBloom(float2 texcoord, float2 offset, float4 params, float4 bloomColor) {
    float4 color = gTexture.Sample(gSampler, texcoord + offset);
    float3 extractedColor = ExtractBloomColor(color.rgb, params) * color.a;
    float3 tintedColor = extractedColor * lerp(float3(1.0f, 1.0f, 1.0f), bloomColor.rgb, bloomColor.a);
    return float4(tintedColor, GetBrightness(extractedColor));
}

/// @brief 画面色にBloomを加算する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Bloom適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);

    float4 bloomParams = GetBloomParams();
    float4 bloomColor = GetBloomColor();
    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));
    float radius = max(bloomParams.z, 0.0f);
    float2 offset = texelSize * radius;

    float4 bloom = 0.0f;
    bloom += SampleBloom(input.texcoord, float2(0.0f, 0.0f), bloomParams, bloomColor) * 0.20f;

    bloom += SampleBloom(input.texcoord, offset * float2(1.0f, 0.0f), bloomParams, bloomColor) * 0.10f;
    bloom += SampleBloom(input.texcoord, offset * float2(-1.0f, 0.0f), bloomParams, bloomColor) * 0.10f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, 1.0f), bloomParams, bloomColor) * 0.10f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, -1.0f), bloomParams, bloomColor) * 0.10f;

    bloom += SampleBloom(input.texcoord, offset * float2(1.0f, 1.0f), bloomParams, bloomColor) * 0.075f;
    bloom += SampleBloom(input.texcoord, offset * float2(-1.0f, 1.0f), bloomParams, bloomColor) * 0.075f;
    bloom += SampleBloom(input.texcoord, offset * float2(1.0f, -1.0f), bloomParams, bloomColor) * 0.075f;
    bloom += SampleBloom(input.texcoord, offset * float2(-1.0f, -1.0f), bloomParams, bloomColor) * 0.075f;

    bloom += SampleBloom(input.texcoord, offset * float2(2.0f, 0.0f), bloomParams, bloomColor) * 0.025f;
    bloom += SampleBloom(input.texcoord, offset * float2(-2.0f, 0.0f), bloomParams, bloomColor) * 0.025f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, 2.0f), bloomParams, bloomColor) * 0.025f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, -2.0f), bloomParams, bloomColor) * 0.025f;

    float intensity = max(bloomParams.x, 0.0f);
    float bloomMask = saturate(bloom.a * intensity);
    output.color = float4(saturate(texColor.rgb + bloom.rgb * intensity), max(texColor.a, bloomMask));
    return output;
}
