#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BinarizeParams : register(b0)
{
    float4 gBinarizeParams;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 二値化エフェクトのパラメータを取得する
/// @return x: しきい値, y: 適用率
float4 GetBinarizeParams()
{
    if (all(gBinarizeParams == 0.0f))
    {
        return float4(0.5f, 1.0f, 0.0f, 0.0f);
    }

    return gBinarizeParams;
}

/// @brief 色の明るさを取得する
/// @param color 入力色
/// @return 明るさ
float GetLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

/// @brief 画面色を白黒のみの二値化色へ変換する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return 二値化後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 params = GetBinarizeParams();
    float threshold = saturate(params.x);
    float intensity = saturate(params.y);

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float luminance = GetLuminance(texColor.rgb);
    float binaryValue = step(threshold, luminance);
    float3 binaryColor = float3(binaryValue, binaryValue, binaryValue);

    output.color = float4(lerp(texColor.rgb, binaryColor, intensity), texColor.a);
    return output;
}
