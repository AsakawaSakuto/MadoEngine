#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer VignetteParams : register(b0)
{
    float4 gVignetteParams;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 画面色を暗く変換する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return 暗く変換したピクセルカラー
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    float2 center = float2(0.5f, 0.5f);
    float dist = length(input.texcoord - center);

    float innerRadius = saturate(gVignetteParams.y);
    float outerRadius = max(innerRadius + 0.0001f, saturate(gVignetteParams.y * gVignetteParams.z));
    float vignette = smoothstep(innerRadius, outerRadius, dist);
    vignette = 1.0f - vignette * saturate(gVignetteParams.x);

    output.color = float4(texColor.rgb * vignette, texColor.a);
    return output;
}
