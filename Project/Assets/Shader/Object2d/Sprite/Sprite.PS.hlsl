#include "Sprite.hlsli"

ConstantBuffer<SpriteMaterial> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(SpriteVertexOutput input)
{
    PixelShaderOutput output;
    
    // Sample the texture
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // Discard pixels with very low alpha to prevent edge bleeding
    if (textureColor.a < 0.01f)
    {
        discard;
    }
    
    // Multiply texture color by material color (includes alpha)
    output.color = textureColor * input.color;
    
    if (output.color.a < 0.01f)
    {
        discard;
    }
    
    return output;
}