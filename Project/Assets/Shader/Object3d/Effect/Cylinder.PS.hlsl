// Compile target: ps_6_0
// RootParameter の対応例:
//   b0 : TransformationMatrix (Cylinder.VS.hlsl)
//   b1 : Material
//   t0 : Texture2D
//   s0 : SamplerState

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer Material : register(b1)
{
    float4 gColor;
    row_major float4x4 gUVTransform;
    float gAlphaReference;
    float3 gPadding;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL0;
};

float4 main(PixelShaderInput input) : SV_TARGET0
{
    float2 texcoord = input.texcoord;

    // gradientLine.png の上下が Cylinder の期待する方向と逆になるため、V を反転します。
    texcoord.y = 1.0f - texcoord.y;

    texcoord = mul(float4(texcoord, 0.0f, 1.0f), gUVTransform).xy;

    float4 color = gTexture.Sample(gSampler, texcoord) * gColor;

    // スライドどおり、透明度 0 の部分だけを捨てます。
    // gAlphaReference を 0.0f にすれば "a == 0.0f" 相当です。
    // フェードの見た目に応じて 0.01f ～ 0.1f 程度に上げることもできます。
    if (color.a <= gAlphaReference)
    {
        discard;
    }

    return color;
}
