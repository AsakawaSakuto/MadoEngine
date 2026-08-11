// CPU側のLine頂点と定数Bufferに一致させる共有構造体
struct LineVertexInput
{
    float3 position : POSITION0;
    float4 color : COLOR0;
};

struct LineVertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

struct LineTransform
{
    float4x4 viewProjection;
};
