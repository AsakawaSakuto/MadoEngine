struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

/// @brief Textureと頂点色を合成してRibbon色を生成
/// @param input Vertex Shaderから受け取った値
/// @return Ribbonの出力色
float4 main(PixelShaderInput input) : SV_TARGET0
{
    float4 color = gTexture.Sample(gSampler, input.uv) * input.color;
    if (color.a <= 0.001f)
    {

        // 微小Alphaを破棄してRibbon外周の不要なBlendを除外
        discard;
    }
    return color;
}
