#include "../../PostEffect/Fog.hlsli"

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    float4 fogColor : COLOR1;
    float fogFactor : TEXCOORD1;
};

struct ParticleInstance
{
    float3 position;
    float rotation;
    float2 scale;
    float2 padding;
    float4 color;
};

StructuredBuffer<ParticleInstance> gParticles : register(t0);

cbuffer PerView : register(b0)
{
    row_major float4x4 gViewProjection;
    float4 gCameraRight;
    float4 gCameraUp;
    float4 gParticleFogColor;
    float4 gParticleFogDistanceParams;
    float4 gParticleFogCameraParams;
};

cbuffer PerBatch : register(b1)
{
    uint gFirstInstance;
    uint gBlendMode;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    ParticleInstance particle = gParticles[gFirstInstance + instanceId];

    float sine = 0.0f;
    float cosine = 1.0f;
    sincos(particle.rotation, sine, cosine);

    float2 scaledPosition = input.position.xy * particle.scale;
    float2 rotatedPosition;
    rotatedPosition.x = scaledPosition.x * cosine - scaledPosition.y * sine;
    rotatedPosition.y = scaledPosition.x * sine + scaledPosition.y * cosine;

    float3 worldPosition = particle.position;
    worldPosition += gCameraRight.xyz * rotatedPosition.x;
    worldPosition += gCameraUp.xyz * rotatedPosition.y;

    VertexShaderOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gViewProjection);
    output.texcoord = input.texcoord;
    output.color = particle.color;
    output.fogColor = gParticleFogColor;
    output.fogFactor = 0.0f;
    if (gParticleFogCameraParams.z > 0.5f)
    {
        float inverseW = rcp(max(abs(output.position.w), 0.0001f));
        float ndcDepth = saturate(output.position.z * inverseW);
        float viewDistance = ConvertDepthToViewDistance(
            ndcDepth,
            gParticleFogCameraParams.x,
            gParticleFogCameraParams.y
        );
        float2 screenTexcoord = float2(
            output.position.x * inverseW * 0.5f + 0.5f,
            0.5f - output.position.y * inverseW * 0.5f
        );
        output.fogFactor = CalculateFogFactor(
            screenTexcoord,
            viewDistance,
            gParticleFogDistanceParams
        );
    }
    return output;
}
