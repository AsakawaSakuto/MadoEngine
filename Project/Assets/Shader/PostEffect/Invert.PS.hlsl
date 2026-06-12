#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 画面色を反転する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return 反転したピクセルカラー
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = float4(1.0f - saturate(texColor.rgb), texColor.a);

    return output;
}
