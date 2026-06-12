#include "CopyImage.hlsli"

Texture2D<float4> gSceneTexture : register(t0);
Texture2D<float4> gEffectTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 sceneColor = gSceneTexture.Sample(gSampler, input.texcoord);
    float4 effectColor = gEffectTexture.Sample(gSampler, input.texcoord);
    float effectMask = saturate(effectColor.a);

    output.color = lerp(sceneColor, effectColor, effectMask);
    output.color.a = 1.0f;
    return output;
}
