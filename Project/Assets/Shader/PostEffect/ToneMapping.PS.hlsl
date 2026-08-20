#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer ToneMappingParams : register(b0) {
    float4 gToneMappingParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief Tone Mappingの調整パラメータを取得
/// @return x: 露出EV, y: Reinhard白レベル, z: ACES適用率, w: Tone Mapper種別
float4 GetToneMappingParams() {
    if (all(gToneMappingParams == 0.0f)) {
        return float4(0.0f, 4.0f, 1.0f, 1.0f);
    }

    return gToneMappingParams;
}

/// @brief 中間色を維持したNeutral Tone Mappingを適用
/// @param hdrColor 露出補正済みHDR色
/// @return 0.8以下を維持して高輝度だけを滑らかに圧縮した色
float3 ApplyNeutralToneMapping(float3 hdrColor) {
    const float shoulderStart = 0.8f;
    const float shoulderRange = 1.0f - shoulderStart;
    float3 compressedColor = shoulderStart + shoulderRange *
        (1.0f - exp(-(hdrColor - shoulderStart) / shoulderRange));
    return lerp(hdrColor, compressedColor, step(shoulderStart, hdrColor));
}

/// @brief 拡張Reinhard Tone Mappingを適用
/// @param hdrColor 露出補正済みHDR色
/// @param whitePoint 白として扱うHDR輝度
/// @return 表示範囲へ圧縮した色
float3 ApplyReinhardToneMapping(float3 hdrColor, float whitePoint) {
    float safeWhitePoint = max(whitePoint, 0.0001f);
    float whitePointSquared = safeWhitePoint * safeWhitePoint;
    return hdrColor * (1.0f + hdrColor / whitePointSquared) / (1.0f + hdrColor);
}

/// @brief ACES Filmic近似Tone Mappingを適用
/// @param hdrColor 露出補正済みHDR色
/// @return 表示範囲へ圧縮した色
float3 ApplyACESToneMapping(float3 hdrColor) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((hdrColor * (a * hdrColor + b)) / (hdrColor * (c * hdrColor + d) + e));
}

/// @brief HDRシーンを表示用LDRへ変換
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Tone Mapping適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 sourceColor = gTexture.Sample(gSampler, input.texcoord);
    float4 params = GetToneMappingParams();

    // EVを2の累乗へ変換して物理量に近い比率でHDR輝度を一括補正
    float exposureScale = exp2(clamp(params.x, -20.0f, 20.0f));
    float3 exposedColor = max(sourceColor.rgb * exposureScale, 0.0f);
    float3 neutralColor = ApplyNeutralToneMapping(exposedColor);
    float3 reinhardColor = ApplyReinhardToneMapping(exposedColor, params.y);
    float3 acesColor = ApplyACESToneMapping(exposedColor);
    float3 filmicColor = lerp(reinhardColor, acesColor, saturate(params.z));
    float toneMapperMode = round(clamp(params.w, 0.0f, 2.0f));

    // 選択されたTone Mapperだけを使用してモード間の意図しない色混合を防止
    float3 toneMappedColor = saturate(exposedColor);
    if (toneMapperMode >= 1.5f) {
        toneMappedColor = filmicColor;
    } else if (toneMapperMode >= 0.5f) {
        toneMappedColor = neutralColor;
    }
    output.color = float4(saturate(toneMappedColor), sourceColor.a);
    return output;
}
