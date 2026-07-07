struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct ShadowTransformationMatrix
{
    float4x4 WVP;
};

ConstantBuffer<ShadowTransformationMatrix> gShadowTransformationMatrix : register(b0);

float4 main(VertexShaderInput input) : SV_POSITION
{
    return mul(input.position, gShadowTransformationMatrix.WVP);
}
