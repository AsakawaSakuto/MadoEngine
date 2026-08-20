#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer CRTParams : register(b0) {
    float4 gCRTDisplayParams;
    float4 gCRTScanlineParams;
    float4 gCRTSurfaceParams;
    float4 gCRTAnimationParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief CRTパラメータが未設定か判定
/// @return すべて未設定の場合はtrue
bool UsesDefaultCRTParams() {
    return all(gCRTDisplayParams == 0.0f) &&
        all(gCRTScanlineParams == 0.0f) &&
        all(gCRTSurfaceParams == 0.0f) &&
        all(gCRTAnimationParams == 0.0f);
}

/// @brief CRTの表示調整パラメータを取得
/// @return x: 全体適用率, y: 画面湾曲, z: 輝度補正, w: 周辺減光
float4 GetCRTDisplayParams() {
    return UsesDefaultCRTParams()
        ? float4(1.0f, 0.08f, 1.03f, 0.25f)
        : gCRTDisplayParams;
}

/// @brief CRTの走査線パラメータを取得
/// @return x: 強度, y: 間隔, z: 太さ, w: 硬さ
float4 GetCRTScanlineParams() {
    return UsesDefaultCRTParams()
        ? float4(0.18f, 3.0f, 0.35f, 4.0f)
        : gCRTScanlineParams;
}

/// @brief CRTの表面模様パラメータを取得
/// @return x: RGBマスク強度, y: RGBマスク幅, z: ノイズ強度, w: ちらつき強度
float4 GetCRTSurfaceParams() {
    return UsesDefaultCRTParams()
        ? float4(0.08f, 1.0f, 0.015f, 0.015f)
        : gCRTSurfaceParams;
}

/// @brief CRTのアニメーションパラメータを取得
/// @return x: 走査速度, y: ちらつき周波数, z: 画面端ぼかし, w: 経過時間
float4 GetCRTAnimationParams() {
    return UsesDefaultCRTParams()
        ? float4(0.0f, 17.0f, 0.015f, 0.0f)
        : gCRTAnimationParams;
}

/// @brief CRT湾曲を適用したサンプリングUVを計算
/// @param texcoord 入力UV
/// @param curvature 湾曲量
/// @return 湾曲後のUV
float2 CalculateCurvedTexcoord(float2 texcoord, float curvature) {
    float2 centered = texcoord * 2.0f - 1.0f;
    float radiusSquared = dot(centered, centered);
    centered *= 1.0f + max(curvature, 0.0f) * radiusSquared;
    return centered * 0.5f + 0.5f;
}

/// @brief 出力ピクセル基準の走査線減衰を計算
/// @param pixelY 出力先のYピクセル座標
/// @param params 走査線パラメータ
/// @param scrollOffset 時間による走査位置補正
/// @return 走査線による輝度倍率
float CalculateScanlineMask(float pixelY, float4 params, float scrollOffset) {
    float spacing = max(params.y, 1.0f);
    float phase = frac((pixelY + scrollOffset) / spacing);
    float distanceToLine = min(phase, 1.0f - phase);
    float halfThickness = min(saturate(params.z) * 0.5f, 0.49f);
    float transitionWidth = max((0.5f - halfThickness) / max(params.w, 0.1f), 0.001f);
    float lineWeight = 1.0f - smoothstep(
        halfThickness,
        min(halfThickness + transitionWidth, 0.5f),
        distanceToLine
    );
    return 1.0f - saturate(params.x) * lineWeight;
}

/// @brief 出力ピクセル基準のRGB Shadow Maskを計算
/// @param pixelX 出力先のXピクセル座標
/// @param intensity RGBマスク強度
/// @param scale 一つの色要素が占めるピクセル幅
/// @return RGB各成分へ乗算するShadow Mask
float3 CalculateShadowMask(float pixelX, float intensity, float scale) {
    const float twoPi = 6.28318530718f;
    const float channelPhase = twoPi / 3.0f;
    float phase = twoPi * pixelX / (3.0f * max(scale, 1.0f));
    float3 pattern = cos(phase + float3(0.0f, channelPhase, channelPhase * 2.0f));
    return 1.0f + pattern * saturate(intensity) * 0.5f;
}

/// @brief 2次元Seedから0以上1未満の疑似乱数を生成
/// @param seed 乱数生成に使用する2次元Seed
/// @return 0以上1未満の疑似乱数
float Hash(float2 seed) {
    return frac(sin(dot(seed, float2(12.9898f, 78.233f))) * 43758.5453f);
}

/// @brief CRT画面の走査線と湾曲を完成画像へ適用
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return CRT表現適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);
    float2 textureSize = float2(max(textureWidth, 1), max(textureHeight, 1));

    float4 displayParams = GetCRTDisplayParams();
    float4 scanlineParams = GetCRTScanlineParams();
    float4 surfaceParams = GetCRTSurfaceParams();
    float4 animationParams = GetCRTAnimationParams();
    float4 sourceColor = gTexture.Sample(gSampler, input.texcoord);
    float2 curvedTexcoord = CalculateCurvedTexcoord(input.texcoord, displayParams.y);

    // 画面外UVをClampしたSampleと境界Maskを分離して湾曲時の色引き延ばしを防止
    float4 curvedColor = gTexture.Sample(gSampler, saturate(curvedTexcoord));
    float2 edgeDistance = min(curvedTexcoord, 1.0f - curvedTexcoord);
    float borderMask = smoothstep(
        0.0f,
        max(animationParams.z, 0.0001f),
        min(edgeDistance.x, edgeDistance.y)
    );

    float2 pixelPosition = curvedTexcoord * textureSize;
    float scanlineMask = CalculateScanlineMask(
        pixelPosition.y,
        scanlineParams,
        animationParams.w * animationParams.x
    );
    float3 shadowMask = CalculateShadowMask(
        pixelPosition.x,
        surfaceParams.x,
        surfaceParams.y
    );

    float2 centered = input.texcoord * 2.0f - 1.0f;
    float vignetteMask = 1.0f - saturate(displayParams.w) *
        saturate(dot(centered, centered) * 0.5f);
    float flicker = 1.0f + sin(
        animationParams.w * max(animationParams.y, 0.0f) * 6.28318530718f
    ) * saturate(surfaceParams.w);
    float noiseFrame = floor(animationParams.w * 60.0f);
    float noise = Hash(floor(pixelPosition) + noiseFrame) - 0.5f;

    // 各CRT要素を表示色空間で合成して最終適用率だけ元画像との補間に使用
    float3 crtColor = curvedColor.rgb * max(displayParams.z, 0.0f);
    crtColor *= scanlineMask * shadowMask * vignetteMask * flicker;
    crtColor += noise * saturate(surfaceParams.z);
    crtColor *= borderMask;

    output.color.rgb = lerp(sourceColor.rgb, saturate(crtColor), saturate(displayParams.x));
    output.color.a = sourceColor.a;
    return output;
}
