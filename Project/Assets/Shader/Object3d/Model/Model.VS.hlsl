#include "Model.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

/// @brief Model頂点をClip空間へ変換してLighting用情報を生成
/// @param input Modelの頂点情報
/// @return Pixel Shaderへ渡すModel情報
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;

    // 非一様Scaleでも法線方向を維持するためWorld逆転置行列を適用
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));

    // Light計算と環境反射をPixel単位で行うためWorld座標を受け渡し
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);

    return output;
}
