struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET0
{
    float4 color = gTexture.Sample(gSampler, input.texcoord) * input.color;
    if (color.a <= 0.001f)
    {
        discard;
    }

    return color;
}
