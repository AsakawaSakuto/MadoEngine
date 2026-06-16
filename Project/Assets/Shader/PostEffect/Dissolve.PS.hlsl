#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float4> gNoiseTexture : register(t3);
SamplerState gSamplerLinear : register(s0);

cbuffer DissolveParams : register(b0) {
    float4 gDissolveParams;
    float4 gDissolveEdgeColor;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief Dissolve用パラメータを取得する
/// @return x: 進行度, y: 境界幅, z: 境界発光強度, w: ノイズ拡大率
float4 GetDissolveParams() {
    if (all(gDissolveParams == 0.0f)) {
        return float4(0.35f, 0.06f, 1.0f, 2.0f);
    }

    return gDissolveParams;
}

/// @brief Dissolve境界色を取得する
/// @return 境界色
float4 GetDissolveEdgeColor() {
    if (all(gDissolveEdgeColor == 0.0f)) {
        return float4(1.0f, 0.45f, 0.05f, 1.0f);
    }

    return gDissolveEdgeColor;
}

/// @brief ノイズ値を取得する
/// @param texcoord 入力UV
/// @param noiseScale ノイズ拡大率
/// @return 0から1のノイズ値
float SampleDissolveNoise(float2 texcoord, float noiseScale) {
    float2 noiseUv = frac(texcoord * max(noiseScale, 0.001f));
    float3 noiseColor = gNoiseTexture.Sample(gSamplerLinear, noiseUv).rgb;
    return dot(noiseColor, float3(0.299f, 0.587f, 0.114f));
}

/// @brief ノイズによるDissolveを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Dissolve適用後の色
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 srcColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    float4 params = GetDissolveParams();
    float4 edgeColor = GetDissolveEdgeColor();

    float dissolveAmount = saturate(params.x);
    float edgeWidth = max(params.y, 0.0001f);
    float edgeIntensity = max(params.z, 0.0f);
    float noiseScale = max(params.w, 0.001f);

    float noise = SampleDissolveNoise(input.texcoord, noiseScale);
    float visibleMask = smoothstep(dissolveAmount - edgeWidth, dissolveAmount + edgeWidth, noise);
    float edgeMask = 1.0f - saturate(abs(noise - dissolveAmount) / edgeWidth);
    edgeMask *= srcColor.a;

    float edgeBlend = saturate(edgeMask * edgeIntensity);
    output.color.rgb = lerp(srcColor.rgb, edgeColor.rgb * max(edgeIntensity, 1.0f), edgeBlend);
    output.color.a = max(srcColor.a * visibleMask, edgeMask * saturate(edgeColor.a));
    return output;
}
