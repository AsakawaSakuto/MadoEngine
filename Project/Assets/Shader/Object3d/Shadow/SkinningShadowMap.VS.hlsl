#include "../Model/Model.hlsli"

StructuredBuffer<Well> gMatrixPalette : register(t1);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

float4 main(SkinningVertexShaderInput input) : SV_POSITION
{
    float4 skinnedPosition = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
    skinnedPosition += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
    skinnedPosition += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
    skinnedPosition += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinnedPosition.w = 1.0f;

    return mul(skinnedPosition, gTransformationMatrix.WVP);
}
