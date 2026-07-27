struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    float4 fogColor : COLOR1;
    float fogFactor : TEXCOORD1;
};

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

cbuffer PerBatch : register(b1)
{
    uint gFirstInstance;
    uint gBlendMode;
};

static const uint kBlendModeNormal = 0;
static const uint kBlendModeAdd = 1;
static const uint kBlendModeSubtract = 2;
static const uint kBlendModeMultiply = 3;
static const uint kBlendModeNone = 4;

/// @brief Particleのブレンド方式に合わせてFogを適用する
/// @param color Fog適用前のParticle色
/// @param fogColor Fog色
/// @param fogFactor 0から1のFog係数
/// @return Fog適用後のParticle色
float3 ApplyParticleFog(float3 color, float3 fogColor, float fogFactor) {
    if (gBlendMode == kBlendModeAdd || gBlendMode == kBlendModeSubtract)
    {
        return color * (1.0f - fogFactor);
    }

    if (gBlendMode == kBlendModeMultiply)
    {
        return lerp(color, float3(1.0f, 1.0f, 1.0f), fogFactor);
    }

    return lerp(color, fogColor, fogFactor);
}

float4 main(PixelShaderInput input) : SV_TARGET0
{
    float4 color = gTexture.Sample(gSampler, input.texcoord) * input.color;
    if (color.a <= 0.001f)
    {
        discard;
    }

    color.rgb = ApplyParticleFog(color.rgb, input.fogColor.rgb, input.fogFactor);
    return color;
}
