#include "Model.hlsli"

StructuredBuffer<Well> gMatrixPalette : register(t1);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

/// @brief 四つのBone行列をWeightで合成して頂点をSkinning
/// @param input Bone IndexとWeightを含むModel頂点
/// @return Skinning後の位置と法線
Skinned Skinning(SkinningVertexShaderInput input)
{
    Skinned skinned;

    // 位置と法線で同じBone Weightを使用してAnimation形状と陰影を一致
    skinned.position = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
    skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
    skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
    skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinned.position.w = 1.0f;

    // 非一様Scaleを含むBone変形でも法線方向を維持するため逆転置行列を合成
    skinned.normal = mul(input.normal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.normal = normalize(skinned.normal);

    return skinned;
}

/// @brief Skinning済みModel頂点をClip空間へ変換
/// @param input Bone情報を含むModel頂点
/// @return Pixel Shaderへ渡すModel情報
VertexShaderOutput main(SkinningVertexShaderInput input)
{
    VertexShaderOutput output;
    Skinned skinned = Skinning(input);

    output.position = mul(skinned.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;

    // Bone空間からModelのWorld空間へ法線と位置を揃えてLightingへ受け渡し
    output.normal = normalize(mul(skinned.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = mul(skinned.position, gTransformationMatrix.World).xyz;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);

    return output;
}
