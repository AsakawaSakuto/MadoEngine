#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer RadialBlurParams : register(b0) {
    float4 gRadialBlurParams;
    float4 gRadialBlurCenterParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief RadialBlurの既定値を使用するかを返す
/// @return 既定値を使用する場合はtrue
bool UsesDefaultRadialBlurParams() {
    return all(gRadialBlurParams == 0.0f) && all(gRadialBlurCenterParams == 0.0f);
}

/// @brief RadialBlurの強度パラメータを返す
/// @return x: 強度, y: サンプル数, z: 半径, w: 距離減衰
float4 GetRadialBlurParams() {
    if (UsesDefaultRadialBlurParams()) {
        return float4(0.45f, 16.0f, 0.45f, 1.0f);
    }

    return gRadialBlurParams;
}

/// @brief RadialBlurの中心座標を返す
/// @return 画面UV上の中心座標
float2 GetRadialBlurCenter() {
    if (UsesDefaultRadialBlurParams()) {
        return float2(0.5f, 0.5f);
    }

    return gRadialBlurCenterParams.xy;
}

/// @brief 中心方向へずらしたUVを作成する
/// @param texcoord 入力UV
/// @param center ブラー中心UV
/// @param sampleRate サンプル位置
/// @param blurLength ブラーの長さ
/// @return サンプリング用UV
float2 CreateRadialBlurSampleTexcoord(float2 texcoord, float2 center, float sampleRate, float blurLength) {
    float2 directionToCenter = center - texcoord;
    return saturate(texcoord + directionToCenter * sampleRate * blurLength);
}

/// @brief 画面色へRadialBlurを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return RadialBlur適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    float4 params = GetRadialBlurParams();
    float intensity = saturate(params.x);
    int sampleCount = clamp((int)floor(params.y + 0.5f), 1, 64);
    float radius = max(params.z, 0.0f);
    float falloff = max(params.w, 0.001f);
    float2 center = GetRadialBlurCenter();

    float2 directionToCenter = center - input.texcoord;
    float distanceFromCenter = length(directionToCenter);
    float blurLength = saturate(pow(saturate(distanceFromCenter * 2.0f), falloff) * radius);

    float4 blurColor = 0.0f;
    float totalWeight = 0.0f;

    [loop]
    for (int sampleIndex = 0; sampleIndex < 64; ++sampleIndex) {
        if (sampleIndex >= sampleCount) {
            break;
        }

        float sampleRate = sampleCount <= 1 ? 0.0f : (float)sampleIndex / (float)(sampleCount - 1);
        float weight = 1.0f - sampleRate * 0.35f;
        float2 sampleTexcoord = CreateRadialBlurSampleTexcoord(input.texcoord, center, sampleRate, blurLength);
        blurColor += gTexture.Sample(gSampler, sampleTexcoord) * weight;
        totalWeight += weight;
    }

    blurColor /= max(totalWeight, 0.0001f);
    output.color = float4(lerp(baseColor.rgb, blurColor.rgb, intensity), baseColor.a);
    return output;
}
