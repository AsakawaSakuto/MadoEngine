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

    // UV変換 + テクスチャカラー取得
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // 共通データ
    float3 N = normalize(input.normal);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

    // === Directional Light ===
    float3 L1 = normalize(-gDirectionalLight.direction);
    float3 halfL1 = normalize(L1 + toEye);
    float NdotL1 = saturate(dot(N, L1));
    float NdotH1 = saturate(dot(N, halfL1));
    float diffuseFactor1 = pow(NdotL1 * 0.5f + 0.5f, 2.0f); 
    if (gDirectionalLight.useHalfLambert != 0)
    {
        diffuseFactor1 = pow(NdotL1 * 0.5f + 0.5f, 2.0f);
    }
    else
    {
        diffuseFactor1 = max(dot(N, L1), 0);
    }
    float specularFactor1 = pow(NdotH1, gMaterial.shininess);

    // === Point Light ===
    float3 L2 = normalize(gPointLight.position - input.worldPosition);
    float distance = length(gPointLight.position - input.worldPosition);
    float attenuation = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decay);
    float3 halfL2 = normalize(L2 + toEye);
    float NdotL2 = saturate(dot(N, L2));
    float NdotH2 = saturate(dot(N, halfL2));
    float diffuseFactor2 = pow(NdotL2 * 0.5f + 0.5f, 2.0f);
    float specularFactor2 = pow(NdotH2, gMaterial.shininess);

    // === Spot Light ===
    float3 spotDirOnSurface = normalize(input.worldPosition - gSpotLight.position); // 入射方向
    float3 L3 = -spotDirOnSurface; // ライト→サーフェス（光の進行方向）

    float spotDistance = length(gSpotLight.position - input.worldPosition);
    float attenuation3 = pow(saturate(-spotDistance / gSpotLight.distance + 1.0), gSpotLight.decay);

    float cosTheta = dot(spotDirOnSurface, normalize(gSpotLight.direction));
    float falloffFactor = saturate((cosTheta - gSpotLight.cosFalloffStart) / (gSpotLight.cosAngle - gSpotLight.cosFalloffStart));

    float3 halfL3 = normalize(L3 + toEye);
    float NdotL3 = saturate(dot(N, L3));
    float NdotH3 = saturate(dot(N, halfL3));
    float diffuseFactor3 = pow(NdotL3 * 0.5f + 0.5f, 2.0f);
    float specularFactor3 = pow(NdotH3, gMaterial.shininess);

    if (gMaterial.enableLighting != 0)
    {
        float3 diffuse1 = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * diffuseFactor1 * gDirectionalLight.intensity;
        float3 specular1 = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularFactor1;

        float3 diffuse2 = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * diffuseFactor2 * gPointLight.intensity * attenuation;
        float3 specular2 = gPointLight.color.rgb * gPointLight.intensity * specularFactor2 * attenuation;

        float3 spotColor = gSpotLight.color.rgb * gSpotLight.intensity * attenuation3 * falloffFactor;
        float3 diffuse3 = gMaterial.color.rgb * textureColor.rgb * spotColor * diffuseFactor3;
        float3 specular3 = spotColor * specularFactor3;

        if (gDirectionalLight.useLight == 0)
        {
            diffuse1 = 0.0f;
            specular1 = 0.0f;
        }
        if (gPointLight.useLight == 0)
        {
            diffuse2 = 0.0f;
            specular2 = 0.0f;
        }
        if (gSpotLight.useLight == 0)
        {
            diffuse3 = 0.0f;
            specular3 = 0.0f;
        }

        // === 環境マッピング（CubeMap 鏡面反射） ===
        float3 environmentReflection = float3(0.0f, 0.0f, 0.0f);
        if (gMaterial.useEnvironmentMap != 0)
        {
            // カメラからピクセル位置へのベクトル（入射ベクトル）
            float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
            // 法線を軸に反射ベクトルを計算
            float3 reflectionVector = reflect(cameraToPosition, N);
            // CubeMap から環境光をサンプリング
            float3 environmentColor = gEnvironmentMap.Sample(gSampler, reflectionVector).rgb;
            // 環境光を鏡面反射成分として加算（強度を抑える）
            environmentReflection = environmentColor * 1.0;
        }

        output.color.rgb = diffuse1 + specular1 + diffuse2 + specular2 + diffuse3 + specular3 + environmentReflection;
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