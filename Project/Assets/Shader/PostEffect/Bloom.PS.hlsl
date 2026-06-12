#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BloomParams : register(b0)
{
    float4 gBloomParams;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 値を返す
/// @return 値
float4 GetBloomParams()
{
    if (all(gBloomParams == 0.0f))
    {
        return float4(0.6f, 0.7f, 4.0f, 0.5f);
    }

    return gBloomParams;
}

/// @brief 色の大きさを返す
/// @param color 色
/// @return 色の大きさ
float GetBrightness(float3 color)
{
    return max(max(color.r, color.g), color.b);
}

/// @brief 入力色を返す
/// @param color 色
/// @param params 値
/// @return 色
float3 ExtractBloomColor(float3 color, float4 params)
{
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

/// @brief ずらした色を返す
/// @param texcoord UV
/// @param offset UV
/// @param params 値
/// @return 色
float4 SampleBloom(float2 texcoord, float2 offset, float4 params)
{
    float4 color = gTexture.Sample(gSampler, texcoord + offset);
    float3 bloomColor = ExtractBloomColor(color.rgb, params) * color.a;
    return float4(bloomColor, GetBrightness(bloomColor));
}

/// @brief 光を足した色を返す
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return 光を足した色
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);

    float4 bloomParams = GetBloomParams();
    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));
    float radius = max(bloomParams.z, 0.0f);
    float2 offset = texelSize * radius;

    float4 bloom = 0.0f;
    bloom += SampleBloom(input.texcoord, float2(0.0f, 0.0f), bloomParams) * 0.20f;

    bloom += SampleBloom(input.texcoord, offset * float2(1.0f, 0.0f), bloomParams) * 0.10f;
    bloom += SampleBloom(input.texcoord, offset * float2(-1.0f, 0.0f), bloomParams) * 0.10f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, 1.0f), bloomParams) * 0.10f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, -1.0f), bloomParams) * 0.10f;

    bloom += SampleBloom(input.texcoord, offset * float2(1.0f, 1.0f), bloomParams) * 0.075f;
    bloom += SampleBloom(input.texcoord, offset * float2(-1.0f, 1.0f), bloomParams) * 0.075f;
    bloom += SampleBloom(input.texcoord, offset * float2(1.0f, -1.0f), bloomParams) * 0.075f;
    bloom += SampleBloom(input.texcoord, offset * float2(-1.0f, -1.0f), bloomParams) * 0.075f;

    bloom += SampleBloom(input.texcoord, offset * float2(2.0f, 0.0f), bloomParams) * 0.025f;
    bloom += SampleBloom(input.texcoord, offset * float2(-2.0f, 0.0f), bloomParams) * 0.025f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, 2.0f), bloomParams) * 0.025f;
    bloom += SampleBloom(input.texcoord, offset * float2(0.0f, -2.0f), bloomParams) * 0.025f;

    float intensity = max(bloomParams.x, 0.0f);
    float bloomMask = saturate(bloom.a * intensity);
    output.color = float4(saturate(texColor.rgb + bloom.rgb * intensity), max(texColor.a, bloomMask));
    return output;
}
