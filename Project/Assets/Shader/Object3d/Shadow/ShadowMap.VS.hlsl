struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct ShadowTransformationMatrix
{
    float4x4 WVP;
};

ConstantBuffer<ShadowTransformationMatrix> gShadowTransformationMatrix : register(b0);

/// @brief Model頂点をShadow Map用のLight Clip空間へ変換
/// @param input Modelの頂点情報
/// @return Light Clip空間の頂点位置
float4 main(VertexShaderInput input) : SV_POSITION
{

    // TexcoordとNormalは通常描画と頂点Layoutを共有するため入力のみ保持
    return mul(input.position, gShadowTransformationMatrix.WVP);
}
