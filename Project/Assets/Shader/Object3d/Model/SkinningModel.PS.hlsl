#include "Model.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b3);
ConstantBuffer<LightGpuData> gLightGpuData : register(b6);
ConstantBuffer<ShadowGpuData> gShadowGpuData : register(b7);

Texture2D<float4> gTexture : register(t0);

// t1をSkinning用Matrix Paletteへ予約するためEnvironment Mapをt2へ配置
TextureCube<float4> gEnvironmentMap : register(t2);
Texture2D<float> gShadowMap : register(t3);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief World座標からShadow Mapの可視率を計算
/// @param worldPosition 判定対象のWorld座標
/// @return 0から1の可視率
float CalculateShadowFactor(float3 worldPosition)
{
    if (gShadowGpuData.useShadow == 0)
    {

        // Shadow無効時は直接光を減衰させない完全可視扱い
        return 1.0f;
    }

    float4 lightClipPosition = mul(float4(worldPosition, 1.0f), gShadowGpuData.lightViewProjection);
    if (lightClipPosition.w <= 0.0f)
    {
        return 1.0f;
    }

    float3 lightNdcPosition = lightClipPosition.xyz / lightClipPosition.w;

    // DirectXのNDC座標を上端原点のTexture UVへ変換
    float2 shadowTexcoord = float2(
        lightNdcPosition.x * 0.5f + 0.5f,
        -lightNdcPosition.y * 0.5f + 0.5f);

    if (shadowTexcoord.x < 0.0f || shadowTexcoord.x > 1.0f ||
        shadowTexcoord.y < 0.0f || shadowTexcoord.y > 1.0f ||
        lightNdcPosition.z < 0.0f || lightNdcPosition.z > 1.0f)
    {

        // Light視錐台外はShadow Mapに対応する遮蔽物がないため可視扱い
        return 1.0f;
    }

    // Receiver Bias付き3x3 PCFで自己Shadowと階段状の境界を緩和
    float2 texelSize = 1.0f / max(gShadowGpuData.shadowMapInfo.xy, float2(1.0f, 1.0f));
    float receiverDepth = lightNdcPosition.z - gShadowGpuData.shadowMapInfo.z;
    float shadow = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            shadow += gShadowMap.SampleCmpLevelZero(
                gShadowSampler,
                shadowTexcoord + float2(x, y) * texelSize,
                receiverDepth);
        }
    }

    return shadow / 9.0f;
}

/// @brief Material、Lighting、Shadow、環境反射からModel色を生成
/// @param input Vertex Shaderから受け取ったModel情報
/// @return Modelの出力色
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    const float kAmbientIntensity = 0.18f;
    const float kSpecularIntensity = 0.18f;
    const float kEnvironmentReflectionIntensity = 0.12f;

    float3 albedo = gMaterial.color.rgb * input.color.rgb * textureColor.rgb;
    float3 N = normalize(input.normal);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

    if (gMaterial.enableLighting != 0)
    {

        // 全Lightの直接光を集計し、Ambientと環境反射は別経路で合成
        LightContribution lightContribution = CalculateLightGpuDataContribution(
            gLightGpuData,
            albedo,
            N,
            input.worldPosition,
            toEye,
            gMaterial.shininess,
            kSpecularIntensity);

        float3 environmentReflection = float3(0.0f, 0.0f, 0.0f);
        if (gMaterial.useEnvironmentMap != 0)
        {
            float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
            float3 reflectionVector = reflect(cameraToPosition, N);
            float3 environmentColor = gEnvironmentMap.Sample(gSampler, reflectionVector).rgb;
            environmentReflection = environmentColor * kEnvironmentReflectionIntensity;
        }

        float shadowFactor = CalculateShadowFactor(input.worldPosition);
        float3 ambient = albedo * kAmbientIntensity;

        // Shadowは直接光だけへ適用してAmbientと環境反射による最低照度を維持
        output.color.rgb = saturate(ambient + (lightContribution.diffuse + lightContribution.specular) * shadowFactor + environmentReflection);
        output.color.a = gMaterial.color.a * input.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * input.color * textureColor;
    }

    if (output.color.a <= 0.0f)
    {

        // 完全透明PixelをDepthとBlendの対象から除外
        discard;
    }

    return output;
}
