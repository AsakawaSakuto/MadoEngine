#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer VignetteParams : register(b0)
{
    float4 gVignetteParams;
    float4 gVignetteColor;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief Vignetteの調整パラメータを取得する
/// @return x: 強度, y: 内側半径, z: 外側倍率, w: 未使用
float4 GetVignetteParams() {
    if (all(gVignetteParams == 0.0f)) {
        return float4(0.8f, 0.35f, 2.0f, 0.0f);
    }

    return gVignetteParams;
}

/// @brief Vignetteの外周色を取得する
/// @return rgb: 外周色, a: 色の適用量
float4 GetVignetteColor() {
    if (all(gVignetteColor == 0.0f)) {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    return gVignetteColor;
}

/// @brief 画面外周に指定色のVignetteを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Vignette適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 vignetteParams = GetVignetteParams();
    float4 vignetteColor = GetVignetteColor();

    float2 center = float2(0.5f, 0.5f);
    float dist = length(input.texcoord - center);

    float innerRadius = saturate(vignetteParams.y);
    float outerRadius = max(innerRadius + 0.0001f, saturate(vignetteParams.y * vignetteParams.z));
    float vignetteWeight = smoothstep(innerRadius, outerRadius, dist);
    vignetteWeight *= saturate(vignetteParams.x) * saturate(vignetteColor.a);

    output.color = float4(lerp(texColor.rgb, saturate(vignetteColor.rgb), vignetteWeight), texColor.a);
    return output;
}
