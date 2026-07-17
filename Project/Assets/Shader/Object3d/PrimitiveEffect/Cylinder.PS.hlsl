struct CylinderInstance
{
    row_major float4x4 world;
    float4 radii;
    float4 geometry;
    float4 uvTransform;
    float4 effectParameters;
    float4 gradientColors[8];
    float4 gradientPositions[2];
    uint4 metadata;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 parameter : TEXCOORD0;
    nointerpolation uint instanceId : TEXCOORD1;
};

StructuredBuffer<CylinderInstance> gInstances : register(t0);
Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

/// @brief Gradient停止位置を取得する
/// @param instance Cylinder Instance
/// @param index Gradient停止位置番号
/// @return 正規化された停止位置
float GetGradientPosition(CylinderInstance instance, uint index)
{
    return instance.gradientPositions[index / 4u][index % 4u];
}

/// @brief 高さに対応するGradient色を評価する
/// @param instance Cylinder Instance
/// @param position 正規化された高さ
/// @return 補間された色
float4 EvaluateGradient(CylinderInstance instance, float position)
{
    const uint count = clamp(instance.metadata.z, 1u, 8u);
    if (count == 1u || position <= GetGradientPosition(instance, 0u))
    {
        return instance.gradientColors[0];
    }

    [loop]
    for (uint index = 0u; index + 1u < count; ++index)
    {
        const float leftPosition = GetGradientPosition(instance, index);
        const float rightPosition = GetGradientPosition(instance, index + 1u);
        if (position <= rightPosition)
        {
            const float range = max(rightPosition - leftPosition, 0.0001f);
            const float rate = saturate((position - leftPosition) / range);
            return lerp(instance.gradientColors[index], instance.gradientColors[index + 1u], rate);
        }
    }

    return instance.gradientColors[count - 1u];
}

/// @brief UV方向設定を正規化パラメータへ適用する
/// @param parameter 円周方向と高さ方向の正規化値
/// @param direction UV方向
/// @return 方向適用後のUV
float2 ResolveBaseUv(float2 parameter, uint direction)
{
    if (direction == 1u)
    {
        return float2(parameter.x, parameter.y);
    }
    if (direction == 2u)
    {
        return float2(parameter.y, parameter.x);
    }
    if (direction == 3u)
    {
        return float2(parameter.y, 1.0f - parameter.x);
    }
    return float2(parameter.x, 1.0f - parameter.y);
}

/// @brief テクスチャ、Gradient、端フェードを合成する
/// @param input Pixel Shader入力
/// @return 出力色
float4 main(PixelShaderInput input) : SV_TARGET0
{
    CylinderInstance instance = gInstances[input.instanceId];
    float2 uv = ResolveBaseUv(input.parameter, instance.metadata.w);

    const float rotation = instance.effectParameters.x;
    float sine = 0.0f;
    float cosine = 1.0f;
    sincos(rotation, sine, cosine);
    uv -= 0.5f;
    uv = float2(
        uv.x * cosine - uv.y * sine,
        uv.x * sine + uv.y * cosine
    );
    uv += 0.5f;
    uv = uv * instance.uvTransform.xy + instance.uvTransform.zw;

    float fade = 1.0f;
    const float bottomFadeRange = instance.effectParameters.y;
    const float topFadeRange = instance.effectParameters.z;
    if (bottomFadeRange > 0.0001f)
    {
        fade *= saturate(input.parameter.y / bottomFadeRange);
    }
    if (topFadeRange > 0.0001f)
    {
        fade *= saturate((1.0f - input.parameter.y) / topFadeRange);
    }

    const float4 gradientColor = EvaluateGradient(instance, saturate(input.parameter.y));
    float4 color = gTexture.Sample(gSampler, uv) * gradientColor;
    color.a *= instance.geometry.w * fade;
    if (color.a <= 0.001f)
    {
        discard;
    }
    return color;
}
