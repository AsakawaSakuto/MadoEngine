// Compile target: vs_6_0
// CPU 側では row-major の DirectXMath / XMFLOAT4X4 をそのまま定数バッファへコピーする前提です。

cbuffer TransformationMatrix : register(b0)
{
    row_major float4x4 gWorldViewProjection;
    row_major float4x4 gWorld;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    output.position = mul(input.position, gWorldViewProjection);
    output.texcoord = input.texcoord;

    // スケールを含む World 行列を使う場合、厳密には逆転置行列を渡すのが正しいです。
    // ここでは通常の回転・平行移動を前提にした簡易版です。
    output.normal = normalize(mul(input.normal, (float3x3)gWorld));

    return output;
}
