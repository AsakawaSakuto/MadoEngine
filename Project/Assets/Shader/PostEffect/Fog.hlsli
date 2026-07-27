#ifndef MADOENGINE_FOG_HLSLI
#define MADOENGINE_FOG_HLSLI

/// @brief 深度値をビュー空間距離へ変換する
/// @param ndcDepth 深度バッファから取得した0から1の深度
/// @param nearClip カメラのニアクリップ距離
/// @param farClip カメラのファークリップ距離
/// @return カメラからのビュー空間距離
float ConvertDepthToViewDistance(float ndcDepth, float nearClip, float farClip) {
    float safeNear = max(nearClip, 0.0001f);
    float safeFar = max(farClip, safeNear + 0.0001f);
    float denominator = max(safeFar - ndcDepth * (safeFar - safeNear), 0.0001f);
    return (safeNear * safeFar) / denominator;
}

/// @brief 距離と画面高さからFog係数を計算する
/// @param texcoord 画面UV
/// @param viewDistance カメラからのビュー空間距離
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

#endif
