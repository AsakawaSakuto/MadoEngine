#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float gray = dot(texColor.rgb, float3(0.2126f, 0.7152f, 0.0722f));

    output.color = float4(gray, gray, gray, texColor.a);
    return output;
}