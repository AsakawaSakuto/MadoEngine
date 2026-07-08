#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer LensDistortionParams : register(b0) {
    float4 gLensDistortionParams;
    float4 gLensDistortionCenterParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief レンズ歪みエフェクトの既定値を使用するかを返す
/// @return 既定値を使用する場合はtrue
bool UsesDefaultLensDistortionParams() {
    return all(gLensDistortionParams == 0.0f) && all(gLensDistortionCenterParams == 0.0f);
}

/// @brief レンズ歪みエフェクトの強度パラメータを取得する
/// @return x: 歪み量, y: 二次歪み量, z: ズーム, w: 適用率
float4 GetLensDistortionParams() {
    if (UsesDefaultLensDistortionParams()) {
        return float4(0.18f, 0.04f, 1.0f, 1.0f);
    }

    return gLensDistortionParams;
}

/// @brief レンズ歪みエフェクトの中心座標を取得する
/// @return 画面UV上の中心座標
float2 GetLensDistortionCenter() {
    if (UsesDefaultLensDistortionParams()) {
        return float2(0.5f, 0.5f);
    }

    return saturate(gLensDistortionCenterParams.xy);
}

/// @brief レンズ歪み後のUVを計算する
/// @param texcoord 入力UV
/// @param center 歪み中心UV
/// @param distortion 歪み量
/// @param cubicDistortion 二次歪み量
/// @param zoom ズーム量
/// @return サンプリング用UV
float2 CalculateLensDistortionTexcoord(
    float2 texcoord,
    float2 center,
    float distortion,
    float cubicDistortion,
    float zoom) {
    float2 direction = texcoord - center;
    float2 normalizedDirection = direction * 2.0f;
    float radiusSquared = dot(normalizedDirection, normalizedDirection);
    float distortionScale = 1.0f + distortion * radiusSquared + cubicDistortion * radiusSquared * radiusSquared;
    float safeZoom = max(zoom, 0.001f);

    return center + direction * distortionScale / safeZoom;
}

/// @brief 画面色へレンズ歪みを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return レンズ歪み適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    float4 params = GetLensDistortionParams();
    float distortion = params.x;
    float cubicDistortion = params.y;
    float zoom = params.z;
    float intensity = saturate(params.w);
    float2 center = GetLensDistortionCenter();

    float2 distortedTexcoord = CalculateLensDistortionTexcoord(input.texcoord, center, distortion, cubicDistortion, zoom);
    float4 distortedColor = gTexture.Sample(gSampler, saturate(distortedTexcoord));

    output.color = float4(lerp(baseColor.rgb, distortedColor.rgb, intensity), baseColor.a);
    return output;
}
