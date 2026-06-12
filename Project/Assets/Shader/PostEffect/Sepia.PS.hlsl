#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 画面色をセピア調に変換する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return セピア調に変換したピクセルカラー
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float3 source = saturate(texColor.rgb);

    float3 sepia;
    sepia.r = dot(source, float3(0.393f, 0.769f, 0.189f));
    sepia.g = dot(source, float3(0.349f, 0.686f, 0.168f));
    sepia.b = dot(source, float3(0.272f, 0.534f, 0.131f));

    output.color = float4(saturate(sepia), texColor.a);
    return output;
}
