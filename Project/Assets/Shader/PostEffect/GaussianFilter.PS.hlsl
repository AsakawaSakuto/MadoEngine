#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer GaussianFilterParams : register(b0) {
    float4 gGaussianFilterParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

static const int kMaxGaussianFilterRadius = 8;
static const float kPi = 3.14159265f;

/// @brief GaussianFilterのパラメータを返す
/// @return x: 標準偏差, y: 半径, z: 適用率
float4 GetGaussianFilterParams() {
    if (all(gGaussianFilterParams == 0.0f)) {
        return float4(1.6f, 2.0f, 1.0f, 0.0f);
    }

    return gGaussianFilterParams;
}

/// @brief 二次元ガウス関数の重みを返す
/// @param x 中心からのX方向距離
/// @param y 中心からのY方向距離
/// @param sigma 標準偏差
/// @return ガウス重み
float CalculateGaussianWeight(float x, float y, float sigma) {
    float sigma2 = max(sigma * sigma, 0.0001f);
    float exponent = -(x * x + y * y) / (2.0f * sigma2);
    return exp(exponent) / (2.0f * kPi * sigma2);
}

/// @brief 指定UVの周辺をガウス重みでぼかした色を返す
/// @param texcoord サンプリング中心UV
/// @param texelSize 1テクセル分のUVサイズ
/// @param sigma 標準偏差
/// @param radius フィルタ半径
/// @return ガウスぼかし後の色
float4 SampleGaussianFilter(float2 texcoord, float2 texelSize, float sigma, int radius) {
    float4 sumColor = 0.0f;
    float totalWeight = 0.0f;

    [loop]
    for (int y = -kMaxGaussianFilterRadius; y <= kMaxGaussianFilterRadius; ++y) {
        if (abs(y) > radius) {
            continue;
        }

        [loop]
        for (int x = -kMaxGaussianFilterRadius; x <= kMaxGaussianFilterRadius; ++x) {
            if (abs(x) > radius) {
                continue;
            }

            float weight = CalculateGaussianWeight((float)x, (float)y, sigma);
            float2 offset = float2((float)x, (float)y) * texelSize;
            sumColor += gTexture.Sample(gSampler, texcoord + offset) * weight;
            totalWeight += weight;
        }
    }

    return sumColor / max(totalWeight, 0.0001f);
}

/// @brief 画面色へGaussianFilterを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return GaussianFilter適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);

    float4 params = GetGaussianFilterParams();
    float sigma = max(params.x, 0.001f);
    int radius = clamp((int)floor(params.y + 0.5f), 1, kMaxGaussianFilterRadius);
    float intensity = saturate(params.z);
    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));

    float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    float4 filterColor = SampleGaussianFilter(input.texcoord, texelSize, sigma, radius);
    output.color = float4(lerp(baseColor.rgb, filterColor.rgb, intensity), baseColor.a);
    return output;
}
