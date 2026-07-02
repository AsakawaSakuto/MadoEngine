struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct SkinningVertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 weight : WEIGHT0;
    int4 index : INDEX0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float4 color : COLOR0;
};

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

struct InstanceData
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 color;
};

struct Material
{
    float4 color;
    int enableLighting;
    int useEnvironmentMap;
    float2 padding1;
    float4x4 uvTransform;
    float shininess;
    float3 padding2;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float pad1;
    float intensity;
    uint useLight;
    uint useHalfLambert;
    float pad2;
};

struct Camera
{
    float3 worldPosition;
    float padding;
};

struct PointLight
{
    float4 color;
    float3 position;
    float pad1;
    float intensity;
    float radius;
    float decay;
    uint useLight;
};

struct SpotLight
{
    float4 color; 
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosFalloffStart;
    uint useLight;
};

#define MAX_DIRECTIONAL_LIGHTS 1
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 8

struct LightGpuData
{
    uint directionalLightCount;
    uint pointLightCount;
    uint spotLightCount;
    uint padding;
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
};

struct LightContribution
{
    float3 diffuse;
    float3 specular;
};

LightContribution MakeEmptyLightContribution()
{
    LightContribution contribution;
    contribution.diffuse = float3(0.0f, 0.0f, 0.0f);
    contribution.specular = float3(0.0f, 0.0f, 0.0f);
    return contribution;
}

LightContribution AddLightContribution(LightContribution baseContribution, LightContribution addContribution)
{
    baseContribution.diffuse += addContribution.diffuse;
    baseContribution.specular += addContribution.specular;
    return baseContribution;
}

LightContribution CalculateDirectionalLightContribution(
    DirectionalLight light,
    float3 albedo,
    float3 normal,
    float3 toEye,
    float shininess,
    float specularIntensity)
{
    LightContribution contribution = MakeEmptyLightContribution();
    if (light.useLight == 0)
    {
        return contribution;
    }

    float3 lightDirection = normalize(-light.direction);
    float3 halfVector = normalize(lightDirection + toEye);
    float normalDotLight = saturate(dot(normal, lightDirection));
    float diffuseFactor = max(dot(normal, lightDirection), 0.0f);
    if (light.useHalfLambert != 0)
    {
        diffuseFactor = pow(normalDotLight * 0.5f + 0.5f, 2.0f);
    }

    float specularFactor = normalDotLight > 0.0f ? pow(saturate(dot(normal, halfVector)), shininess) : 0.0f;
    contribution.diffuse = albedo * light.color.rgb * diffuseFactor * light.intensity;
    contribution.specular = light.color.rgb * light.intensity * specularFactor * specularIntensity;
    return contribution;
}

LightContribution CalculatePointLightContribution(
    PointLight light,
    float3 albedo,
    float3 normal,
    float3 worldPosition,
    float3 toEye,
    float shininess,
    float specularIntensity)
{
    LightContribution contribution = MakeEmptyLightContribution();
    if (light.useLight == 0 || light.radius <= 0.0f)
    {
        return contribution;
    }

    float3 toLight = light.position - worldPosition;
    float3 lightDirection = normalize(toLight);
    float distanceToLight = length(toLight);
    float attenuation = pow(saturate(-distanceToLight / light.radius + 1.0f), light.decay);
    float3 halfVector = normalize(lightDirection + toEye);
    float normalDotLight = saturate(dot(normal, lightDirection));
    float diffuseFactor = pow(normalDotLight * 0.5f + 0.5f, 2.0f);
    float specularFactor = normalDotLight > 0.0f ? pow(saturate(dot(normal, halfVector)), shininess) : 0.0f;

    contribution.diffuse = albedo * light.color.rgb * diffuseFactor * light.intensity * attenuation;
    contribution.specular = light.color.rgb * light.intensity * specularFactor * attenuation * specularIntensity;
    return contribution;
}

LightContribution CalculateSpotLightContribution(
    SpotLight light,
    float3 albedo,
    float3 normal,
    float3 worldPosition,
    float3 toEye,
    float shininess,
    float specularIntensity)
{
    LightContribution contribution = MakeEmptyLightContribution();
    if (light.useLight == 0 || light.distance <= 0.0f || light.cosAngle == light.cosFalloffStart)
    {
        return contribution;
    }

    float3 spotDirectionOnSurface = normalize(worldPosition - light.position);
    float3 lightDirection = -spotDirectionOnSurface;
    float spotDistance = length(light.position - worldPosition);
    float attenuation = pow(saturate(-spotDistance / light.distance + 1.0f), light.decay);
    float cosTheta = dot(spotDirectionOnSurface, normalize(light.direction));
    float outerCosAngle = min(light.cosAngle, light.cosFalloffStart);
    float innerCosAngle = max(light.cosAngle, light.cosFalloffStart);
    float falloffFactor = saturate((cosTheta - outerCosAngle) / (innerCosAngle - outerCosAngle));
    float3 halfVector = normalize(lightDirection + toEye);
    float normalDotLight = saturate(dot(normal, lightDirection));
    float diffuseFactor = pow(normalDotLight * 0.5f + 0.5f, 2.0f);
    float specularFactor = normalDotLight > 0.0f ? pow(saturate(dot(normal, halfVector)), shininess) : 0.0f;
    float3 spotColor = light.color.rgb * light.intensity * attenuation * falloffFactor;

    contribution.diffuse = albedo * spotColor * diffuseFactor;
    contribution.specular = spotColor * specularFactor * specularIntensity;
    return contribution;
}

LightContribution CalculateLightGpuDataContribution(
    LightGpuData lightData,
    float3 albedo,
    float3 normal,
    float3 worldPosition,
    float3 toEye,
    float shininess,
    float specularIntensity)
{
    LightContribution totalContribution = MakeEmptyLightContribution();

    uint directionalCount = min(lightData.directionalLightCount, MAX_DIRECTIONAL_LIGHTS);
    for (uint index = 0; index < directionalCount; ++index)
    {
        totalContribution = AddLightContribution(
            totalContribution,
            CalculateDirectionalLightContribution(
                lightData.directionalLights[index],
                albedo,
                normal,
                toEye,
                shininess,
                specularIntensity));
    }

    uint pointCount = min(lightData.pointLightCount, MAX_POINT_LIGHTS);
    for (uint index = 0; index < pointCount; ++index)
    {
        totalContribution = AddLightContribution(
            totalContribution,
            CalculatePointLightContribution(
                lightData.pointLights[index],
                albedo,
                normal,
                worldPosition,
                toEye,
                shininess,
                specularIntensity));
    }

    uint spotCount = min(lightData.spotLightCount, MAX_SPOT_LIGHTS);
    for (uint index = 0; index < spotCount; ++index)
    {
        totalContribution = AddLightContribution(
            totalContribution,
            CalculateSpotLightContribution(
                lightData.spotLights[index],
                albedo,
                normal,
                worldPosition,
                toEye,
                shininess,
                specularIntensity));
    }

    return totalContribution;
}

struct Skinned
{
    float4 position;
    float3 normal;
};

struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};
