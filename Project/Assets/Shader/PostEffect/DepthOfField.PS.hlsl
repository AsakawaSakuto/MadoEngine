#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gSceneDepthTexture : register(t1);
SamplerState gSamplerLinear : register(s0);
SamplerState gSamplerPoint : register(s1);

cbuffer DepthOfFieldParams : register(b0) {
    float4 gDofFocusParams;
    float4 gDofCameraParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

static const int kDofSampleCount = 16;
static const float2 kDofSampleOffsets[kDofSampleCount] = {
    float2(-0.326f, -0.406f),
    float2(-0.840f, -0.074f),
    float2(-0.696f, 0.457f),
    float2(-0.203f, 0.621f),
    float2(0.962f, -0.195f),
    float2(0.473f, -0.480f),
    float2(0.519f, 0.767f),
    float2(0.185f, -0.893f),
    float2(0.507f, 0.064f),
    float2(0.896f, 0.412f),
    float2(-0.322f, -0.933f),
    float2(-0.792f, -0.598f),
    float2(-0.095f, -0.126f),
    float2(-0.648f, 0.127f),
    float2(-0.166f, 0.318f),
    float2(0.401f, -0.230f),
};

/// @brief Depth Of Fieldの焦点パラメータを取得する
/// @return x: 焦点距離, y: 焦点幅, z: ぼかし半径, w: 適用率
float4 GetDofFocusParams() {
    if (all(gDofFocusParams == 0.0f)) {
        return float4(300.0f, 120.0f, 8.0f, 1.0f);
    }

    return gDofFocusParams;
}

/// @brief Depth Of Fieldのカメラパラメータを取得する
/// @return x: NearClip, y: FarClip, z: 手前ぼけ強度, w: 奥ぼけ強度
float4 GetDofCameraParams() {
    if (all(gDofCameraParams == 0.0f)) {
        return float4(0.1f, 1000.0f, 1.0f, 1.0f);
    }

    return gDofCameraParams;
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

/// @brief Depthが背景クリア値に近いか判定する
/// @param depth 判定する深度
/// @return 背景に近い場合はtrue
bool IsClearDepth(float depth) {
    return depth >= 0.9999f;
}

/// @brief 焦点距離からのずれをぼけ量へ変換する
/// @param viewDistance CameraからのView空間距離
/// @param focusParams 焦点パラメータ
/// @param cameraParams カメラパラメータ
/// @return 0から1のぼけ量
float CalculateCircleOfConfusion(float viewDistance, float4 focusParams, float4 cameraParams) {
    float focusDistance = max(focusParams.x, 0.0f);
    float focusRange = max(focusParams.y, 0.0001f);
    float distanceDelta = viewDistance - focusDistance;
    float sideStrength = distanceDelta < 0.0f ? cameraParams.z : cameraParams.w;
    float blurRate = smoothstep(0.0f, focusRange, abs(distanceDelta));
    return saturate(blurRate * max(sideStrength, 0.0f));
}

/// @brief 指定UV周辺をぼけ量に応じてサンプリングする
/// @param texcoord サンプリング中心UV
/// @param texelSize 1ピクセル分のUVサイズ
/// @param blurRadius ぼかし半径
/// @param coc 0から1のぼけ量
/// @return 周辺サンプルを合成した色
float4 SampleDepthOfFieldBlur(float2 texcoord, float2 texelSize, float blurRadius, float coc) {
    float2 scaledRadius = texelSize * max(blurRadius, 0.0f) * coc;
    float4 sumColor = gTexture.Sample(gSamplerLinear, texcoord);
    float totalWeight = 1.0f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < kDofSampleCount; ++sampleIndex) {
        float2 offset = kDofSampleOffsets[sampleIndex];
        float weight = 1.0f - saturate(length(offset) * 0.35f);
        float2 sampleTexcoord = saturate(texcoord + offset * scaledRadius);
        sumColor += gTexture.Sample(gSamplerLinear, sampleTexcoord) * weight;
        totalWeight += weight;
    }

    return sumColor / max(totalWeight, 0.0001f);
}

/// @brief 深度に応じたDepth Of Fieldを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Depth Of Field適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);

    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));
    float4 focusParams = GetDofFocusParams();
    float4 cameraParams = GetDofCameraParams();

    float4 baseColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    float sceneDepth = gSceneDepthTexture.Sample(gSamplerPoint, input.texcoord);
    float viewDistance = IsClearDepth(sceneDepth)
        ? max(cameraParams.y, cameraParams.x + 0.0001f)
        : ConvertDepthToViewDistance(sceneDepth, cameraParams.x, cameraParams.y);
    float coc = CalculateCircleOfConfusion(viewDistance, focusParams, cameraParams);
    float4 blurColor = SampleDepthOfFieldBlur(input.texcoord, texelSize, focusParams.z, coc);

    output.color = float4(lerp(baseColor.rgb, blurColor.rgb, coc * saturate(focusParams.w)), baseColor.a);
    return output;
}
