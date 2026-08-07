struct VertexShaderInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer PerView : register(b0)
{
    row_major float4x4 gViewProjection;
};

/// @brief World座標のRibbon頂点をViewProjection変換する
/// @param input CPUで生成されたRibbon頂点
/// @return Pixel Shaderへ渡す頂点
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(float4(input.position, 1.0f), gViewProjection);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
