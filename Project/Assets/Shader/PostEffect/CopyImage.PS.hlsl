#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 入力TextureをAlphaを含めて出力先へ複製
/// @param input Fullscreen Triangleから受け取った画面UV
/// @return 入力Textureと同一のPixel色
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // PostEffect間の中間BufferとしてAlpha Channelも変更せず維持
    output.color = gTexture.Sample(gSampler, input.texcoord);
    return output;
}
