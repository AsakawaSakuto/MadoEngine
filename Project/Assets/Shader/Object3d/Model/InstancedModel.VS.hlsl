#include "Model.hlsli"

StructuredBuffer<InstanceData> gInstanceData : register(t2);

/// @brief Instanceごとの行列と色でModel頂点を変換
/// @param input Modelの頂点情報
/// @param instanceId 描画対象のInstance番号
/// @return Pixel Shaderへ渡すModel情報
VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{

    // 行列と色はInstance番号をKeyに同一要素から取得
    InstanceData instance = gInstanceData[instanceId];

    VertexShaderOutput output;
    output.position = mul(input.position, instance.WVP);
    output.texcoord = input.texcoord;

    // 非一様Scaleでも法線方向を維持するためInstance別の逆転置行列を適用
    output.normal = normalize(mul(input.normal, (float3x3) instance.WorldInverseTranspose));
    output.worldPosition = mul(input.position, instance.World).xyz;
    output.color = instance.color;

    return output;
}
