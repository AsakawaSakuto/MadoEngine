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

/// @brief Particleのブレンド方式に合わせてFogを適用
/// @param color Fog適用前のParticle色
/// @param fogColor Fog色
/// @param fogFactor 0から1のFog係数
/// @return Fog適用後のParticle色
float3 ApplyParticleFog(float3 color, float3 fogColor, float fogFactor) {
    if (gBlendMode == kBlendModeAdd || gBlendMode == kBlendModeSubtract)
    {

        // 加減算Blendの中立色である黒へ減衰して背景色への不要な加算を防止
        return color * (1.0f - fogFactor);
    }

    if (gBlendMode == kBlendModeMultiply)
    {

        // 乗算Blendの中立色である白へ近づけて遠景の暗化を抑制
        return lerp(color, float3(1.0f, 1.0f, 1.0f), fogFactor);
    }

    return lerp(color, fogColor, fogFactor);
}

/// @brief Texture色とParticle色を合成してBlend方式別のFogを適用
/// @param input Vertex Shaderから受け取ったParticle情報
/// @return Particleの出力色
float4 main(PixelShaderInput input) : SV_TARGET0
{
    float4 color = gTexture.Sample(gSampler, input.texcoord) * input.color;
    if (color.a <= 0.001f)
    {

        // 微小Alphaを破棄してBlend対象とTexture境界の色漏れを削減
        discard;
    }

    color.rgb = ApplyParticleFog(color.rgb, input.fogColor.rgb, input.fogFactor);
    return color;
}
