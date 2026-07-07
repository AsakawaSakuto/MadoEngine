#include "Model.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b3);
ConstantBuffer<LightGpuData> gLightGpuData : register(b6);
ConstantBuffer<ShadowGpuData> gShadowGpuData : register(b7);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentMap : register(t2);
Texture2D<float> gShadowMap : register(t3);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float CalculateShadowFactor(float3 worldPosition)
{
    if (gShadowGpuData.useShadow == 0)
    {
        return 1.0f;
    }

    float4 lightClipPosition = mul(float4(worldPosition, 1.0f), gShadowGpuData.lightViewProjection);
    if (lightClipPosition.w <= 0.0f)
    {
        return 1.0f;
    }

    float3 lightNdcPosition = lightClipPosition.xyz / lightClipPosition.w;
    float2 shadowTexcoord = float2(
        lightNdcPosition.x * 0.5f + 0.5f,
        -lightNdcPosition.y * 0.5f + 0.5f);

    if (shadowTexcoord.x < 0.0f || shadowTexcoord.x > 1.0f ||
        shadowTexcoord.y < 0.0f || shadowTexcoord.y > 1.0f ||
        lightNdcPosition.z < 0.0f || lightNdcPosition.z > 1.0f)
    {
        return 1.0f;
    }

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
        output.color.rgb = saturate(ambient + (lightContribution.diffuse + lightContribution.specular) * shadowFactor + environmentReflection);
        output.color.a = gMaterial.color.a * input.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * input.color * textureColor;
    }

    if (output.color.a <= 0.0f)
    {
        discard;
    }

    return output;
}
