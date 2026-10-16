#include "../Sprite/Sprite.hlsli"

struct TextMaterial {
    float4 color;
    float4x4 uvTransformMatrix;
    float4 outlineColor;
    float2 textureSize;
    float sdfRange;
    float outlineWidth;
};

ConstantBuffer<TextMaterial> gTextMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

/// @brief 通常の文字画像または単一チャンネルSDFから文字と縁を合成
/// @param input 共通Sprite頂点Shaderからの文字座標と色
/// @return 透明度を反映した文字と縁取りの出力色
PixelShaderOutput main(SpriteVertexOutput input) {
    PixelShaderOutput output;
    const float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    if (gTextMaterial.sdfRange <= 0.0f) {

        // SDF未使用の既存Textも同じPSOで描画し、混在時の設定切り替えを回避
        output.color = textureColor * input.color;
    } else {

        // 距離を画面Pixelへ換算し、拡縮や回転中も指定した縁幅とAA幅を維持
        const float2 texturePosition = input.texcoord * gTextMaterial.textureSize;
        const float texelsPerPixel = max(max(length(ddx(texturePosition)), length(ddy(texturePosition))), 0.0001f);
        const float pixelDistance = (textureColor.a - 0.5f) * (2.0f * gTextMaterial.sdfRange) / texelsPerPixel;
        const float availableWidth = max(0.0f, (gTextMaterial.sdfRange - 1.0f) / texelsPerPixel);
        const float outlineWidth = min(gTextMaterial.outlineWidth, availableWidth);
        const float fill = saturate(pixelDistance + 0.5f);
        const float outer = saturate(pixelDistance + outlineWidth + 0.5f);
        const float outline = (outer - fill) * gTextMaterial.outlineColor.a;
        const float coverage = fill + outline;

        // Straight Alphaへ戻して既存Blendと整合し、文字のFadeを縁にも適用
        output.color.rgb = (input.color.rgb * fill + gTextMaterial.outlineColor.rgb * outline) / max(coverage, 0.0001f);
        output.color.a = coverage * input.color.a;
    }
    if (output.color.a < 0.01f) {
        discard;
    }
    return output;
}
