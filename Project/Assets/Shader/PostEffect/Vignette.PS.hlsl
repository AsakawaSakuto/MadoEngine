#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer VignetteParams : register(b0) {
    float4 gVignetteParams;
    float4 gVignetteColor;
    float4 gVignetteTransform;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief Vignetteの調整パラメータを取得
/// @return x: 強度, y: 内側半径, z: 外側倍率, w: 形状番号（0は楕円、1は長方形）
float4 GetVignetteParams() {
    if (all(gVignetteParams == 0.0f)) {
        return float4(0.8f, 0.35f, 2.0f, 0.0f);
    }

    return gVignetteParams;
}

/// @brief Vignetteの外周色を取得
/// @return rgb: 外周色, a: 色の適用量
float4 GetVignetteColor() {
    if (all(gVignetteColor == 0.0f)) {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    return gVignetteColor;
}

/// @brief Vignetteの中心位置と軸別スケールを取得
/// @return xy: 画面UV上の中心位置, zw: XY方向の拡大率
float4 GetVignetteTransform() {

    // 拡張領域が未設定の場合は従来の中心位置と等倍スケールを維持
    if (all(gVignetteTransform == 0.0f)) {
        return float4(0.5f, 0.5f, 1.0f, 1.0f);
    }

    return gVignetteTransform;
}

/// @brief 選択形状に応じたVignetteの中心距離を算出
/// @param offset 中心位置と軸別スケールを反映した相対UV
/// @param shape 形状番号、0は楕円、1は長方形
/// @return 内外半径との比較に使用する中心距離
float CalculateVignetteDistance(float2 offset, float shape) {

    // 最大軸距離で等距離線を長方形化し、楕円と同じ半径およびぼかし範囲を共用
    if (shape >= 0.5f) {
        float2 absoluteOffset = abs(offset);
        return max(absoluteOffset.x, absoluteOffset.y);
    }

    return length(offset);
}

/// @brief 画面外周に指定色のVignetteを適用
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Vignette適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 vignetteParams = GetVignetteParams();
    float4 vignetteColor = GetVignetteColor();
    float4 vignetteTransform = GetVignetteTransform();

    // 中心からの相対UVを軸別スケールで除算して明るい領域を拡縮、最小値でゼロ除算を回避
    float2 scale = max(vignetteTransform.zw, float2(0.0001f, 0.0001f));
    float2 offset = (input.texcoord - vignetteTransform.xy) / scale;
    float dist = CalculateVignetteDistance(offset, vignetteParams.w);

    float innerRadius = saturate(vignetteParams.y);

    // 内外半径に最小差を確保してSmoothstepの未定義範囲を回避
    float outerRadius = max(innerRadius + 0.0001f, saturate(vignetteParams.y * vignetteParams.z));
    float vignetteWeight = smoothstep(innerRadius, outerRadius, dist);
    vignetteWeight *= saturate(vignetteParams.x) * saturate(vignetteColor.a);

    output.color = float4(lerp(texColor.rgb, saturate(vignetteColor.rgb), vignetteWeight), texColor.a);
    return output;
}
