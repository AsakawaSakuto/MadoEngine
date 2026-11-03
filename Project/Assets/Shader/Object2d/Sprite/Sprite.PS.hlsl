#include "Sprite.hlsli"

ConstantBuffer<SpriteMaterial> gMaterial : register(b0);

struct SpriteSampling {
    uint textureFilter;
};

ConstantBuffer<SpriteSampling> gSampling : register(b2);
Texture2D<float4> gTexture : register(t0);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler : register(s1);

/// @brief Texture色とMaterial色からSpriteの出力色を生成
/// @param input Vertex Shaderから受け取ったSprite情報
/// @return Spriteの出力色
PixelShaderOutput main(SpriteVertexOutput input) {
    PixelShaderOutput output;

    // Sprite::TextureFilterと選択値を一致させ、Draw単位で一様な分岐により補間方式を選択
    float4 textureColor;
    if (gSampling.textureFilter == 1u) {
        textureColor = gTexture.Sample(gPointSampler, input.texcoord);
    } else {
        textureColor = gTexture.Sample(gLinearSampler, input.texcoord);
    }

    // Texture境界の補間色による縁取りを防ぐため微小Alphaを破棄
    if (textureColor.a < 0.01f) {
        discard;
    }
    output.color = textureColor * input.color;

    // Material側のAlpha反映後も完全に透明なPixelを描画対象から除外
    if (output.color.a < 0.01f) {
        discard;
    }
    
    return output;
}
