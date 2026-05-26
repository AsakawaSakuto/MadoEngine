#include "Line.hlsli"

ConstantBuffer<LineTransform> gTransform : register(b0);

LineVertexOutput main(LineVertexInput input)
{
    LineVertexOutput output;
    
    float4 worldPos = float4(input.position, 1.0f);
    output.position = mul(worldPos, gTransform.viewProjection);
    output.color = input.color;
    
    return output;
}