#include "Line.hlsli"

ConstantBuffer<LineTransform> gTransform : register(b0);

/// @brief World座標のLine頂点をClip空間へ変換
/// @param input CPUで生成されたLine頂点
/// @return Pixel Shaderへ渡すLine情報
LineVertexOutput main(LineVertexInput input)
{
    LineVertexOutput output;

    // Line頂点はCPU側でWorld座標まで確定しているためViewProjectionのみ適用
    float4 worldPos = float4(input.position, 1.0f);
    output.position = mul(worldPos, gTransform.viewProjection);
    output.color = input.color;

    return output;
}
