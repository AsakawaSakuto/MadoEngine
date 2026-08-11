#include "Sprite.hlsli"

ConstantBuffer<SpriteMaterial> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

/// @brief Texture色とMaterial色からSpriteの出力色を生成
/// @param input Vertex Shaderから受け取ったSprite情報
/// @return Spriteの出力色
PixelShaderOutput main(SpriteVertexOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // Texture境界の補間色による縁取りを防ぐため微小Alphaを破棄
    if (textureColor.a < 0.01f)
    {
        discard;
    }
    output.color = textureColor * input.color;

    // Material側のAlpha反映後も完全に透明なPixelを描画対象から除外
    if (output.color.a < 0.01f)
    {
        discard;
    }
    
    return output;
}
