#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer ColorFilterParams : register(b0) {
    float4 gColorFilterParams;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

/// @brief カラーフィルターのパラメータを取得する
/// @return x: フィルター色R, y: フィルター色G, z: フィルター色B, w: 適用率
float4 GetColorFilterParams() {
    if (all(gColorFilterParams == 0.0f)) {
        return float4(1.0f, 0.85f, 0.65f, 1.0f);
    }

    return gColorFilterParams;
}

/// @brief 画面色へ指定色のカラーフィルターを適用する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return カラーフィルター適用後のピクセルカラー
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 params = GetColorFilterParams();
    float3 filterColor = saturate(params.rgb);
    float intensity = saturate(params.w);
    float3 filteredColor = texColor.rgb * filterColor;

    output.color = float4(lerp(texColor.rgb, filteredColor, intensity), texColor.a);
    return output;
}
