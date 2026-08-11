#include "Sprite.hlsli"

ConstantBuffer<SpriteMaterial> gMaterial : register(b0);
ConstantBuffer<SpriteTransformationMatrix> gTransform : register(b1);

/// @brief Sprite頂点をClip空間へ変換
/// @param input Spriteの頂点情報
/// @return Pixel Shaderへ渡すSprite情報
SpriteVertexOutput main(SpriteVertexInput input)
{
    SpriteVertexOutput output;
    output.position = mul(input.position, gTransform.WVP);

    // Texture Atlasの移動や拡縮を頂点段階でUVへ反映
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransformMatrix);
    output.texcoord = transformedUV.xy;
    output.color = gMaterial.color;

    return output;
}
