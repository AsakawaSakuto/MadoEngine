#include "Model.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<Camera> gCamera : register(b3);
ConstantBuffer<PointLight> gPointLight : register(b4);
ConstantBuffer<SpotLight> gSpotLight : register(b5);

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

    // UV変換とテクスチャカラーを取得する
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
        float3 diffuse1 = float3(0.0f, 0.0f, 0.0f);
        float3 specular1 = float3(0.0f, 0.0f, 0.0f);
        float3 diffuse2 = float3(0.0f, 0.0f, 0.0f);
        float3 specular2 = float3(0.0f, 0.0f, 0.0f);
        float3 diffuse3 = float3(0.0f, 0.0f, 0.0f);
        float3 specular3 = float3(0.0f, 0.0f, 0.0f);

        // 平行光源
        if (gDirectionalLight.useLight != 0)
        {
            float3 L1 = normalize(-gDirectionalLight.direction);
            float3 halfL1 = normalize(L1 + toEye);
            float NdotL1 = saturate(dot(N, L1));
            float diffuseFactor1 = max(dot(N, L1), 0.0f);
            if (gDirectionalLight.useHalfLambert != 0)
            {
                diffuseFactor1 = pow(NdotL1 * 0.5f + 0.5f, 2.0f);
            }
            float specularFactor1 = pow(saturate(dot(N, halfL1)), gMaterial.shininess);

            diffuse1 = albedo * gDirectionalLight.color.rgb * diffuseFactor1 * gDirectionalLight.intensity;
            specular1 = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularFactor1 * kSpecularIntensity;
        }

        // 点光源
        if (gPointLight.useLight != 0 && gPointLight.radius > 0.0f)
        {
            float3 L2 = normalize(gPointLight.position - input.worldPosition);
            float distance = length(gPointLight.position - input.worldPosition);
            float attenuation = pow(saturate(-distance / gPointLight.radius + 1.0f), gPointLight.decay);
            float3 halfL2 = normalize(L2 + toEye);
            float diffuseFactor2 = pow(saturate(dot(N, L2)) * 0.5f + 0.5f, 2.0f);
            float specularFactor2 = pow(saturate(dot(N, halfL2)), gMaterial.shininess);

            diffuse2 = albedo * gPointLight.color.rgb * diffuseFactor2 * gPointLight.intensity * attenuation;
            specular2 = gPointLight.color.rgb * gPointLight.intensity * specularFactor2 * attenuation * kSpecularIntensity;
        }

        // スポットライト
        if (gSpotLight.useLight != 0 && gSpotLight.distance > 0.0f && gSpotLight.cosAngle != gSpotLight.cosFalloffStart)
        {
            float3 spotDirOnSurface = normalize(input.worldPosition - gSpotLight.position);
            float3 L3 = -spotDirOnSurface;
            float spotDistance = length(gSpotLight.position - input.worldPosition);
            float attenuation3 = pow(saturate(-spotDistance / gSpotLight.distance + 1.0f), gSpotLight.decay);
            float cosTheta = dot(spotDirOnSurface, normalize(gSpotLight.direction));
            float falloffFactor = saturate((cosTheta - gSpotLight.cosFalloffStart) / (gSpotLight.cosAngle - gSpotLight.cosFalloffStart));
            float3 halfL3 = normalize(L3 + toEye);
            float diffuseFactor3 = pow(saturate(dot(N, L3)) * 0.5f + 0.5f, 2.0f);
            float specularFactor3 = pow(saturate(dot(N, halfL3)), gMaterial.shininess);

            float3 spotColor = gSpotLight.color.rgb * gSpotLight.intensity * attenuation3 * falloffFactor;
            diffuse3 = albedo * spotColor * diffuseFactor3;
            specular3 = spotColor * specularFactor3 * kSpecularIntensity;
        }

        float3 environmentReflection = float3(0.0f, 0.0f, 0.0f);
        if (gMaterial.useEnvironmentMap != 0)
        {
            float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
            float3 reflectionVector = reflect(cameraToPosition, N);
            float3 environmentColor = gEnvironmentMap.Sample(gSampler, reflectionVector).rgb;
            environmentReflection = environmentColor * kEnvironmentReflectionIntensity;
        }

        float3 ambient = albedo * kAmbientIntensity;
        output.color.rgb = saturate(ambient + diffuse1 + specular1 + diffuse2 + specular2 + diffuse3 + specular3 + environmentReflection);
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
