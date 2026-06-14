#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer PixelArtParams : register(b0)
{
    float4 gPixelArtParams;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief ドット絵風エフェクトのパラメータを取得する
/// @return x: ピクセルサイズ, y: 色階調数, z: コントラスト, w: 適用率
float4 GetPixelArtParams()
{
    if (all(gPixelArtParams == 0.0f))
    {
        return float4(6.0f, 8.0f, 1.15f, 1.0f);
    }

    return gPixelArtParams;
}

/// @brief UVを粗いピクセル単位へ丸める
/// @param texcoord 入力UV
/// @param textureSize テクスチャサイズ
/// @param pixelSize ドット絵風にするための画面上のピクセル幅
/// @return 丸めたUV
float2 SnapTexcoordToPixelGrid(float2 texcoord, float2 textureSize, float pixelSize)
{
    float2 pixelCoord = texcoord * textureSize;
    float2 snappedPixelCoord = floor(pixelCoord / pixelSize) * pixelSize + pixelSize * 0.5f;
    return saturate(snappedPixelCoord / textureSize);
}

/// @brief 色数を減らしてドット絵風の階調へ変換する
/// @param color 入力色
/// @param colorSteps 色階調数
/// @return 減色した色
float3 QuantizeColor(float3 color, float colorSteps)
{
    float steps = max(floor(colorSteps), 2.0f);
    float maxIndex = steps - 1.0f;
    return floor(saturate(color) * maxIndex + 0.5f) / maxIndex;
}

/// @brief 中間色を基準にコントラストを調整する
/// @param color 入力色
/// @param contrast コントラスト
/// @return コントラスト調整後の色
float3 ApplyContrast(float3 color, float contrast)
{
    return saturate((color - 0.5f) * max(contrast, 0.0f) + 0.5f);
}

/// @brief 画面色をドット絵風に変換する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return ドット絵風に変換したピクセルカラー
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 params = GetPixelArtParams();
    float pixelSize = max(floor(params.x), 1.0f);
    float colorSteps = max(floor(params.y), 2.0f);
    float contrast = max(params.z, 0.0f);
    float intensity = saturate(params.w);

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);
    float2 textureSize = float2(max(textureWidth, 1), max(textureHeight, 1));

    float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    float2 snappedTexcoord = SnapTexcoordToPixelGrid(input.texcoord, textureSize, pixelSize);
    float4 pixelColor = gTexture.Sample(gSampler, snappedTexcoord);

    float3 stylizedColor = QuantizeColor(pixelColor.rgb, colorSteps);
    stylizedColor = ApplyContrast(stylizedColor, contrast);

    output.color = float4(lerp(baseColor.rgb, stylizedColor, intensity), baseColor.a);
    return output;
}
