#include "Model.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b3);
ConstantBuffer<LightGpuData> gLightGpuData : register(b6);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentMap : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    const float kAmbientIntensity = 0.18f;
    const float kSpecularIntensity = 0.18f;
    const float kEnvironmentReflectionIntensity = 0.12f;

    float3 albedo = gMaterial.color.rgb * textureColor.rgb;
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

        float3 ambient = albedo * kAmbientIntensity;
        output.color.rgb = saturate(ambient + lightContribution.diffuse + lightContribution.specular + environmentReflection);
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }

    if (output.color.a <= 0.0f)
    {
        discard;
    }

    return output;
}
