#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer FXAAParams : register(b0) {
    float4 gFXAAParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief FXAAの調整パラメータを取得
/// @return x: 相対エッジしきい値, y: 最小エッジしきい値, z: 輪郭探索範囲, w: 適用率
float4 GetFXAAParams() {
    if (all(gFXAAParams == 0.0f)) {
        return float4(0.125f, 0.0312f, 8.0f, 1.0f);
    }

    return gFXAAParams;
}

/// @brief RGBから知覚輝度を計算
/// @param color 輝度を計算するRGB
/// @return Rec.709係数による知覚輝度
float GetLuminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

/// @brief 指定UVの画面色を取得
/// @param texcoord サンプリングするUV
/// @return 指定UVのRGB
float3 SampleColor(float2 texcoord) {
    return gTexture.Sample(gSampler, saturate(texcoord)).rgb;
}

/// @brief 輝度勾配に沿ってアンチエイリアス色を計算
/// @param texcoord 対象ピクセルのUV
/// @param texelSize 1ピクセル分のUVサイズ
/// @param searchSpan 輪郭方向の最大探索範囲
/// @param lumaMin 近傍の最小輝度
/// @param lumaMax 近傍の最大輝度
/// @param lumaNorthWest 左上ピクセルの輝度
/// @param lumaNorthEast 右上ピクセルの輝度
/// @param lumaSouthWest 左下ピクセルの輝度
/// @param lumaSouthEast 右下ピクセルの輝度
/// @return 輪郭方向へ平滑化したRGB
float3 CalculateFXAAColor(
    float2 texcoord,
    float2 texelSize,
    float searchSpan,
    float lumaMin,
    float lumaMax,
    float lumaNorthWest,
    float lumaNorthEast,
    float lumaSouthWest,
    float lumaSouthEast) {
    float2 direction;
    direction.x = -((lumaNorthWest + lumaNorthEast) - (lumaSouthWest + lumaSouthEast));
    direction.y = (lumaNorthWest + lumaSouthWest) - (lumaNorthEast + lumaSouthEast);

    // 低コントラスト領域で方向ベクトルが過剰に増幅されないよう輝度に応じて減衰
    float averageLuma = (lumaNorthWest + lumaNorthEast + lumaSouthWest + lumaSouthEast) * 0.25f;
    float directionReduce = max(averageLuma * 0.125f, 1.0f / 128.0f);
    float inverseMinDirection = 1.0f / (min(abs(direction.x), abs(direction.y)) + directionReduce);
    direction = clamp(direction * inverseMinDirection, -searchSpan, searchSpan) * texelSize;

    // 輪郭内側の二点を合成して短いエッジや細線の過剰なぼかしを抑制
    float3 innerColor = 0.5f * (
        SampleColor(texcoord + direction * (1.0f / 3.0f - 0.5f)) +
        SampleColor(texcoord + direction * (2.0f / 3.0f - 0.5f))
    );

    // 輪郭両端を追加した候補から輝度範囲を外れる色のにじみを除外
    float3 outerColor = innerColor * 0.5f + 0.25f * (
        SampleColor(texcoord - direction * 0.5f) +
        SampleColor(texcoord + direction * 0.5f)
    );
    float outerLuma = GetLuminance(outerColor);
    if (outerLuma < lumaMin || outerLuma > lumaMax) {
        return innerColor;
    }

    return outerColor;
}

/// @brief 完成画像へFXAAを適用
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return FXAA適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);
    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));

    float4 params = GetFXAAParams();
    float edgeThreshold = max(params.x, 0.0f);
    float minEdgeThreshold = max(params.y, 0.0f);
    float searchSpan = clamp(params.z, 1.0f, 32.0f);
    float intensity = saturate(params.w);

    float4 sourceColor = gTexture.Sample(gSampler, input.texcoord);
    float lumaCenter = GetLuminance(sourceColor.rgb);
    float lumaNorthWest = GetLuminance(SampleColor(input.texcoord + texelSize * float2(-1.0f, -1.0f)));
    float lumaNorthEast = GetLuminance(SampleColor(input.texcoord + texelSize * float2(1.0f, -1.0f)));
    float lumaSouthWest = GetLuminance(SampleColor(input.texcoord + texelSize * float2(-1.0f, 1.0f)));
    float lumaSouthEast = GetLuminance(SampleColor(input.texcoord + texelSize * float2(1.0f, 1.0f)));

    float lumaMin = min(lumaCenter, min(min(lumaNorthWest, lumaNorthEast), min(lumaSouthWest, lumaSouthEast)));
    float lumaMax = max(lumaCenter, max(max(lumaNorthWest, lumaNorthEast), max(lumaSouthWest, lumaSouthEast)));
    float lumaRange = lumaMax - lumaMin;

    // 平坦な領域を早期終了して不要なサンプリングとテクスチャのぼけを回避
    float requiredRange = max(minEdgeThreshold, lumaMax * edgeThreshold);
    if (lumaRange < requiredRange || intensity <= 0.0f) {
        output.color = sourceColor;
        return output;
    }

    float3 fxaaColor = CalculateFXAAColor(
        input.texcoord,
        texelSize,
        searchSpan,
        lumaMin,
        lumaMax,
        lumaNorthWest,
        lumaNorthEast,
        lumaSouthWest,
        lumaSouthEast
    );
    output.color = float4(lerp(sourceColor.rgb, fxaaColor, intensity), sourceColor.a);
    return output;
}
