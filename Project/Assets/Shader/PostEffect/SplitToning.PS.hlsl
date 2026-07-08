#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer SplitToningParams : register(b0) {
    float4 gSplitToningShadowColor;
    float4 gSplitToningHighlightColor;
    float4 gSplitToningParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief Split Toningの暗部色を取得する
/// @return rgb: 暗部に加える色, a: 暗部への適用量
float4 GetSplitToningShadowColor() {
    if (all(gSplitToningShadowColor == 0.0f)) {
        return float4(0.12f, 0.25f, 0.75f, 0.45f);
    }

    return gSplitToningShadowColor;
}

/// @brief Split Toningの明部色を取得する
/// @return rgb: 明部に加える色, a: 明部への適用量
float4 GetSplitToningHighlightColor() {
    if (all(gSplitToningHighlightColor == 0.0f)) {
        return float4(1.0f, 0.72f, 0.35f, 0.35f);
    }

    return gSplitToningHighlightColor;
}

/// @brief Split Toningの調整パラメータを取得する
/// @return x: 分岐位置, y: なじみ幅, z: 全体適用率, w: 輝度保持率
float4 GetSplitToningParams() {
    if (all(gSplitToningParams == 0.0f)) {
        return float4(0.0f, 0.2f, 1.0f, 0.75f);
    }

    return gSplitToningParams;
}

/// @brief 色の輝度を取得する
/// @param color 輝度を求める色
/// @return Rec.709係数による輝度
float GetLuminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

/// @brief 指定色でトーンを乗せる
/// @param source 元の色
/// @param tintColor 乗せる色
/// @param preserveLuminance 輝度保持率
/// @return トーン適用後の色
float3 ApplyToneColor(float3 source, float3 tintColor, float preserveLuminance) {
    float sourceLuminance = max(GetLuminance(source), 0.0001f);
    float3 tonedColor = saturate(source * max(tintColor, 0.0f) * 2.0f);
    float tonedLuminance = max(GetLuminance(tonedColor), 0.0001f);
    float3 luminancePreservedColor = saturate(tonedColor * (sourceLuminance / tonedLuminance));

    return lerp(tonedColor, luminancePreservedColor, saturate(preserveLuminance));
}

/// @brief 暗部と明部に別々の色味を加える
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Split Toning適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float3 source = saturate(texColor.rgb);

    float4 shadowColor = GetSplitToningShadowColor();
    float4 highlightColor = GetSplitToningHighlightColor();
    float4 params = GetSplitToningParams();

    float balance = saturate(params.x * 0.5f + 0.5f);
    float softness = max(params.y, 0.001f);
    float intensity = saturate(params.z);
    float preserveLuminance = saturate(params.w);
    float luminance = saturate(GetLuminance(source));

    float highlightWeight = smoothstep(balance - softness, balance + softness, luminance);
    float shadowWeight = 1.0f - highlightWeight;

    float3 shadowTone = ApplyToneColor(source, saturate(shadowColor.rgb), preserveLuminance);
    float3 highlightTone = ApplyToneColor(source, saturate(highlightColor.rgb), preserveLuminance);

    float3 tonedColor = source;
    tonedColor = lerp(tonedColor, shadowTone, shadowWeight * saturate(shadowColor.a));
    tonedColor = lerp(tonedColor, highlightTone, highlightWeight * saturate(highlightColor.a));

    output.color = float4(lerp(source, tonedColor, intensity), texColor.a);
    return output;
}
