#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BoxFilterParams : register(b0) {
    float4 gBoxFilterParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

static const int kMaxBoxFilterRadius = 8;

/// @brief BoxFilterのパラメータを返す
/// @return x: 半径, y: 適用率
float4 GetBoxFilterParams() {
    if (all(gBoxFilterParams == 0.0f)) {
        return float4(1.0f, 1.0f, 0.0f, 0.0f);
    }

    return gBoxFilterParams;
}

/// @brief 指定UVの周辺を平均化した色を返す
/// @param texcoord サンプリング中心UV
/// @param texelSize 1テクセル分のUVサイズ
/// @param radius フィルタ半径
/// @return 平均化した色
float4 SampleBoxFilter(float2 texcoord, float2 texelSize, int radius) {
    float4 sumColor = 0.0f;
    float sampleCount = 0.0f;

    [loop]
    for (int y = -kMaxBoxFilterRadius; y <= kMaxBoxFilterRadius; ++y) {
        if (abs(y) > radius) {
            continue;
        }

        [loop]
        for (int x = -kMaxBoxFilterRadius; x <= kMaxBoxFilterRadius; ++x) {
            if (abs(x) > radius) {
                continue;
            }

            float2 offset = float2((float)x, (float)y) * texelSize;
            sumColor += gTexture.Sample(gSampler, texcoord + offset);
            sampleCount += 1.0f;
        }
    }

    return sumColor / max(sampleCount, 1.0f);
}

/// @brief 画面色へBoxFilterを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return BoxFilter適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);

    float4 params = GetBoxFilterParams();
    int radius = clamp((int)floor(params.x + 0.5f), 1, kMaxBoxFilterRadius);
    float intensity = saturate(params.y);
    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));

    float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    float4 filterColor = SampleBoxFilter(input.texcoord, texelSize, radius);
    output.color = float4(lerp(baseColor.rgb, filterColor.rgb, intensity), baseColor.a);
    return output;
}
