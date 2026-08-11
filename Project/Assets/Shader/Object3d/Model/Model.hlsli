// CPU側のModel用頂点、定数Buffer、StructuredBufferと同一Layoutを維持
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

// CPU側LightManagerの固定配列上限と一致させるLight数
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

struct ShadowGpuData
{
    float4x4 lightViewProjection;
    float4 shadowMapInfo;
    uint useShadow;
    float3 padding;
};

struct LightContribution
{
    float3 diffuse;
    float3 specular;
};

/// @brief 拡散反射と鏡面反射を持たないLight寄与を生成
/// @return 全成分がゼロのLight寄与
LightContribution MakeEmptyLightContribution()
{
    LightContribution contribution;
    contribution.diffuse = float3(0.0f, 0.0f, 0.0f);
    contribution.specular = float3(0.0f, 0.0f, 0.0f);
    return contribution;
}

/// @brief 二つのLight寄与を成分ごとに加算
/// @param baseContribution 加算先のLight寄与
/// @param addContribution 加算するLight寄与
/// @return 加算後のLight寄与
LightContribution AddLightContribution(LightContribution baseContribution, LightContribution addContribution)
{
    baseContribution.diffuse += addContribution.diffuse;
    baseContribution.specular += addContribution.specular;
    return baseContribution;
}

/// @brief Directional Light一灯分の反射寄与を計算
/// @param light Directional Light情報
/// @param albedo MaterialとTextureを反映した基本色
/// @param normal World空間の単位法線
/// @param toEye 頂点からCameraへ向かう単位Vector
/// @param shininess 鏡面反射の鋭さ
/// @param specularIntensity 鏡面反射の強度
/// @return 拡散反射と鏡面反射の寄与
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

    // 設定時のみ背面側へ柔らかく回り込むHalf Lambertへ切り替え
    if (light.useHalfLambert != 0)
    {
        diffuseFactor = pow(normalDotLight * 0.5f + 0.5f, 2.0f);
    }

    float specularFactor = normalDotLight > 0.0f ? pow(saturate(dot(normal, halfVector)), shininess) : 0.0f;
    contribution.diffuse = albedo * light.color.rgb * diffuseFactor * light.intensity;
    contribution.specular = light.color.rgb * light.intensity * specularFactor * specularIntensity;
    return contribution;
}

/// @brief Point Light一灯分の反射寄与を計算
/// @param light Point Light情報
/// @param albedo MaterialとTextureを反映した基本色
/// @param normal World空間の単位法線
/// @param worldPosition 描画点のWorld座標
/// @param toEye 描画点からCameraへ向かう単位Vector
/// @param shininess 鏡面反射の鋭さ
/// @param specularIntensity 鏡面反射の強度
/// @return 距離減衰を含む拡散反射と鏡面反射の寄与
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

    // Radius外をゼロに固定した減衰でLightの影響範囲を有限化
    float attenuation = pow(saturate(-distanceToLight / light.radius + 1.0f), light.decay);
    float3 halfVector = normalize(lightDirection + toEye);
    float normalDotLight = saturate(dot(normal, lightDirection));
    float diffuseFactor = pow(normalDotLight * 0.5f + 0.5f, 2.0f);
    float specularFactor = normalDotLight > 0.0f ? pow(saturate(dot(normal, halfVector)), shininess) : 0.0f;

    contribution.diffuse = albedo * light.color.rgb * diffuseFactor * light.intensity * attenuation;
    contribution.specular = light.color.rgb * light.intensity * specularFactor * attenuation * specularIntensity;
    return contribution;
}

/// @brief Spot Light一灯分の反射寄与を計算
/// @param light Spot Light情報
/// @param albedo MaterialとTextureを反映した基本色
/// @param normal World空間の単位法線
/// @param worldPosition 描画点のWorld座標
/// @param toEye 描画点からCameraへ向かう単位Vector
/// @param shininess 鏡面反射の鋭さ
/// @param specularIntensity 鏡面反射の強度
/// @return 距離減衰と角度減衰を含む反射寄与
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

    // 内外角の入力順に依存しないようCos値を大小へ正規化
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

/// @brief GPUへ転送された全Lightの反射寄与を集計
/// @param lightData 全Light情報
/// @param albedo MaterialとTextureを反映した基本色
/// @param normal World空間の単位法線
/// @param worldPosition 描画点のWorld座標
/// @param toEye 描画点からCameraへ向かう単位Vector
/// @param shininess 鏡面反射の鋭さ
/// @param specularIntensity 鏡面反射の強度
/// @return 有効な全Lightの反射寄与
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

    // CPU側Countが上限を超えても固定長配列外を参照しないよう制限
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
