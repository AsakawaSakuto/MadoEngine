// Fullscreen描画のVertex Shaderと各PostEffectで共有するClip座標とUV
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};
