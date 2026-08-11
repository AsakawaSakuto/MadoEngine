#include "CopyImage.hlsli"

Texture2D<float4> gSceneTexture : register(t0);
Texture2D<float4> gEffectTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief Effect TextureのAlphaをMaskとしてScene色へ合成
/// @param input Fullscreen Triangleから受け取った画面UV
/// @return Effect合成後の不透明色
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 sceneColor = gSceneTexture.Sample(gSampler, input.texcoord);
    float4 effectColor = gEffectTexture.Sample(gSampler, input.texcoord);

    // Effect側Alphaを出力透明度ではなくSceneとの合成率として使用
    float effectMask = saturate(effectColor.a);

    output.color = lerp(sceneColor, effectColor, effectMask);

    // 後続のBackBuffer合成で再度透明Blendされないよう最終Alphaを固定
    output.color.a = 1.0f;
    return output;
}
