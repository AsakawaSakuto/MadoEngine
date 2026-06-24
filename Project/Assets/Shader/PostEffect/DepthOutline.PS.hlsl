#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gSceneDepthTexture : register(t1);
Texture2D<float> gMaskDepthTexture : register(t2);
SamplerState gSamplerLinear : register(s0);
SamplerState gSamplerPoint : register(s1);

cbuffer OutlineParams : register(b0)
{
    float4 gOutlineColor;
    float4 gOutlineParams;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

static const float kPrewittHorizontal[3][3] = {
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float kPrewittVertical[3][3] = {
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

/// @brief アウトラインの太さを取得する
/// @return アウトラインの太さ
float GetOutlineThickness()
{
    if (gOutlineParams.x <= 0.0f)
    {
        return 1.0f;
    }

    return max(gOutlineParams.x, 0.25f);
}

/// @brief 深度エッジの感度を取得する
/// @return 深度エッジの感度
float GetDepthSensitivity()
{
    if (gOutlineParams.y <= 0.0f)
    {
        return 80.0f;
    }

    return gOutlineParams.y;
}

/// @brief エッジ検出のしきい値を取得する
/// @return エッジ検出のしきい値
float GetEdgeThreshold()
{
    if (gOutlineParams.z <= 0.0f)
    {
        return 0.005f;
    }

    return gOutlineParams.z;
}

/// @brief アウトラインの濃さを取得する
/// @return アウトラインの濃さ
float GetOutlineIntensity()
{
    if (gOutlineParams.w <= 0.0f)
    {
        return 1.0f;
    }

    return gOutlineParams.w;
}

/// @brief 深度が背景クリア値に近いか判定する
/// @param depth 判定する深度
/// @return 背景に近い場合はtrue
bool IsClearDepth(float depth)
{
    return depth >= 0.9999f;
}

/// @brief マスク深度がシーン深度より手前か判定する
/// @param maskDepth マスク側の深度
/// @param sceneDepth シーン側の深度
/// @return マスクが見えている場合はtrue
bool IsMaskVisible(float maskDepth, float sceneDepth)
{
    return IsClearDepth(sceneDepth) || maskDepth <= sceneDepth + 0.0002f;
}

/// @brief Prewittフィルタで深度差分を計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @param thickness アウトラインの太さ
/// @return 深度差分
float2 CalculateDepthDifference(float2 texcoord, float2 texelSize, float thickness)
{
    float2 difference = float2(0.0f, 0.0f);

    [unroll]
    for (int x = 0; x < 3; ++x)
    {
        [unroll]
        for (int y = 0; y < 3; ++y)
        {
            float2 offset = float2(x - 1, y - 1) * texelSize * thickness;
            float depth = gMaskDepthTexture.Sample(gSamplerPoint, texcoord + offset);
            difference.x += depth * kPrewittHorizontal[x][y];
            difference.y += depth * kPrewittVertical[x][y];
        }
    }

    return difference;
}

/// @brief Prewittフィルタでアルファ差分を計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @param thickness アウトラインの太さ
/// @return アルファ差分
float2 CalculateAlphaDifference(float2 texcoord, float2 texelSize, float thickness)
{
    float2 difference = float2(0.0f, 0.0f);

    [unroll]
    for (int x = 0; x < 3; ++x)
    {
        [unroll]
        for (int y = 0; y < 3; ++y)
        {
            float2 offset = float2(x - 1, y - 1) * texelSize * thickness;
            float alpha = gTexture.Sample(gSamplerPoint, texcoord + offset).a;
            difference.x += alpha * kPrewittHorizontal[x][y];
            difference.y += alpha * kPrewittVertical[x][y];
        }
    }

    return difference;
}

/// @brief 差分から滑らかなエッジ重みを計算する
/// @param edgeValue エッジ差分値
/// @param threshold しきい値
/// @return 0から1のエッジ重み
float CalculateSmoothEdgeWeight(float edgeValue, float threshold)
{
    float smoothWidth = max(threshold * 2.0f, 0.001f);
    return smoothstep(threshold, threshold + smoothWidth, edgeValue);
}

/// @brief アウトラインの重みを計算する
/// @param texcoord サンプリングするUV座標
/// @param texelSize 1ピクセル分のUVサイズ
/// @return アウトラインの重み
float CalculateOutlineWeight(float2 texcoord, float2 texelSize)
{
    float thickness = GetOutlineThickness();
    float threshold = GetEdgeThreshold();

    float2 depthDifference = CalculateDepthDifference(texcoord, texelSize, thickness);
    float depthEdge = length(depthDifference) * GetDepthSensitivity();
    float depthWeight = CalculateSmoothEdgeWeight(depthEdge, threshold);

    float2 alphaDifference = CalculateAlphaDifference(texcoord, texelSize, thickness);
    float alphaEdge = length(alphaDifference);
    float alphaWeight = CalculateSmoothEdgeWeight(alphaEdge, 0.02f);

    float centerDepth = gMaskDepthTexture.Sample(gSamplerPoint, texcoord);
    float depthGate = IsClearDepth(centerDepth) ? alphaWeight : saturate(alphaWeight * 2.0f + depthWeight * 0.25f);
    float outlineWeight = max(alphaWeight, depthWeight * depthGate);
    return saturate(outlineWeight * GetOutlineIntensity());
}

/// @brief 深度とアルファのPrewittアウトラインを描画する
/// @param input 頂点シェーダーから受け取った画面座標とUV
/// @return アウトライン適用後の色
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    uint textureWidth = 0;
    uint textureHeight = 0;
    gTexture.GetDimensions(textureWidth, textureHeight);

    float2 texelSize = 1.0f / float2(max(textureWidth, 1), max(textureHeight, 1));
    float outlineWeight = CalculateOutlineWeight(input.texcoord, texelSize);

    float4 srcColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    float maskDepth = gMaskDepthTexture.Sample(gSamplerPoint, input.texcoord);
    float sceneDepth = gSceneDepthTexture.Sample(gSamplerPoint, input.texcoord);
    float visibleBody = IsMaskVisible(maskDepth, sceneDepth) ? srcColor.a : 0.0f;
    float outputAlpha = max(visibleBody, outlineWeight);

    output.color.rgb = lerp(srcColor.rgb, gOutlineColor.rgb, outlineWeight);
    output.color.a = outputAlpha;
    return output;
}
