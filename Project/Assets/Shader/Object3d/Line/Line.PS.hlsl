#include "Line.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 補間された頂点色をLineの出力色として使用
/// @param input Vertex Shaderから受け取ったLine情報
/// @return Lineの出力色
PixelShaderOutput main(LineVertexOutput input)
{
    PixelShaderOutput output;
    output.color = input.color;
    return output;
}
