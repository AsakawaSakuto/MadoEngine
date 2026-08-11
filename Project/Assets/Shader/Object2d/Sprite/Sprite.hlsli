// CPU側のSprite定数Bufferと入出力Layoutに一致させる共有構造体
struct SpriteVertexInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

struct SpriteVertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct SpriteMaterial
{
    float4 color;
    float4x4 uvTransformMatrix;
};

struct SpriteTransformationMatrix
{
    float4x4 WVP;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
