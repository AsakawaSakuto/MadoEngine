#include "../Model/Model.hlsli"

StructuredBuffer<Well> gMatrixPalette : register(t1);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

/// @brief Skinning後の頂点をShadow Map用のLight Clip空間へ変換
/// @param input Bone IndexとWeightを含むModel頂点
/// @return Light Clip空間の頂点位置
float4 main(SkinningVertexShaderInput input) : SV_POSITION
{

    // 通常描画と同じ4Boneの線形BlendでShadow形状をAnimationへ追従
    float4 skinnedPosition = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
    skinnedPosition += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
    skinnedPosition += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
    skinnedPosition += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinnedPosition.w = 1.0f;

    // Weight誤差で変動し得る同次成分を位置Vectorとして固定してからLight空間へ変換
    return mul(skinnedPosition, gTransformationMatrix.WVP);
}
