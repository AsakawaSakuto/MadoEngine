#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer RandomParams : register(b0) {
    float4 gRandomParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief Random用パラメータを取得する
/// @return x: 時間, y: ノイズ拡大率, z: コントラスト, w: 適用率
float4 GetRandomParams() {
    if (all(gRandomParams == 0.0f)) {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    return gRandomParams;
}

/// @brief 2次元Seedから0以上1未満の疑似乱数を生成する
/// @param seed 乱数生成に使用する2次元Seed
/// @return 0以上1未満の疑似乱数
float Rand2dTo1d(float2 seed) {
    float random = sin(dot(seed, float2(12.9898f, 78.233f))) * 43758.5453f;
    return frac(random);
}

/// @brief 入力UVと時間から白黒乱数値を生成する
/// @param texcoord 入力UV
/// @param params Random用パラメータ
/// @return 0から1の白黒乱数値
float CalculateRandomValue(float2 texcoord, float4 params) {
    float time = max(params.x, 0.0001f);
    float noiseScale = max(params.y, 0.0001f);
    float contrast = max(params.z, 0.0f);

    float2 seed = texcoord * noiseScale * time;
    float random = Rand2dTo1d(seed);
    return saturate((random - 0.5f) * contrast + 0.5f);
}

/// @brief GPUで生成した白黒乱数を画面へ出力する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return Random適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 params = GetRandomParams();
    float4 srcColor = gTexture.Sample(gSampler, input.texcoord);
    float random = CalculateRandomValue(input.texcoord, params);
    float intensity = saturate(params.w);
    float3 randomColor = float3(random, random, random);

    output.color.rgb = lerp(srcColor.rgb, randomColor, intensity);
    output.color.a = srcColor.a;
    return output;
}
