#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 画面色を知覚輝度に基づくGray Scaleへ変換
/// @param input Fullscreen Triangleから受け取った画面UV
/// @return Gray Scale変換後のPixel色
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    // RGBの知覚差を反映するRec.709係数で明るさを維持
    float gray = dot(texColor.rgb, float3(0.2126f, 0.7152f, 0.0722f));

    output.color = float4(gray, gray, gray, texColor.a);
    return output;
}
