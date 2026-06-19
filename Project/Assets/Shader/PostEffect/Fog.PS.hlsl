#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gSceneDepthTexture : register(t1);
SamplerState gSampler : register(s0);
SamplerState gPointSampler : register(s1);

cbuffer FogParams : register(b0) {
    float4 gFogColor;
    float4 gFogDistanceParams;
    float4 gFogCameraParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief Fogの色を取得する
/// @return Fog色
float4 GetFogColor() {
    if (all(gFogColor == 0.0f)) {
        return float4(0.58f, 0.68f, 0.74f, 1.0f);
    }

    return gFogColor;
}

/// @brief Fogパラメータを取得する
/// @return x: 開始距離, y: 終了距離, z: 濃度, w: 高さ方向の強度
float4 GetFogDistanceParams() {
    if (all(gFogDistanceParams == 0.0f)) {
        return float4(850.0f, 1000.0f, 1.0f, 0.0f);
    }

    return gFogDistanceParams;
}

/// @brief Cameraパラメータを取得する
/// @return x: NearClip, y: FarClip, z: 未使用, w: 未使用
float4 GetFogCameraParams() {
    if (all(gFogCameraParams == 0.0f)) {
        return float4(0.1f, 1000.0f, 0.0f, 0.0f);
    }

    return gFogCameraParams;
}

/// @brief Depth値をView空間距離へ変換する
/// @param ndcDepth DepthBufferから取得した0から1の深度
/// @param nearClip CameraのNearClip距離
/// @param farClip CameraのFarClip距離
/// @return CameraからのView空間距離
float ConvertDepthToViewDistance(float ndcDepth, float nearClip, float farClip) {
    float safeNear = max(nearClip, 0.0001f);
    float safeFar = max(farClip, safeNear + 0.0001f);
    float denominator = max(safeFar - ndcDepth * (safeFar - safeNear), 0.0001f);
    return (safeNear * safeFar) / denominator;
}

/// @brief 距離と画面高さからFog係数を計算する
/// @param texcoord 入力UV
/// @param viewDistance CameraからのView空間距離
/// @param distanceParams Fog距離パラメータ
/// @return 0から1のFog係数
float CalculateFogFactor(float2 texcoord, float viewDistance, float4 distanceParams) {
    float startDistance = max(distanceParams.x, 0.0f);
    float endDistance = max(distanceParams.y, startDistance + 0.0001f);
    float density = max(distanceParams.z, 0.0f);
    float heightStrength = max(distanceParams.w, 0.0f);

    float distanceFactor = smoothstep(startDistance, endDistance, viewDistance);
    float heightFactor = pow(saturate(1.0f - texcoord.y), 1.5f) * heightStrength;
    return saturate((distanceFactor + heightFactor) * density);
}

/// @brief 深度に応じたFogを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Fog適用後の色
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 srcColor = gTexture.Sample(gSampler, input.texcoord);
    float sceneDepth = gSceneDepthTexture.Sample(gPointSampler, input.texcoord);
    float4 fogColor = GetFogColor();
    float4 distanceParams = GetFogDistanceParams();
    float4 cameraParams = GetFogCameraParams();
    float viewDistance = ConvertDepthToViewDistance(sceneDepth, cameraParams.x, cameraParams.y);
    float fogFactor = CalculateFogFactor(input.texcoord, viewDistance, distanceParams);

    output.color.rgb = lerp(srcColor.rgb, fogColor.rgb, fogFactor);
    output.color.a = srcColor.a;
    return output;
}
