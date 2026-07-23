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
};

cbuffer PerBatch : register(b1)
{
    uint gFirstInstance;
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
    return output;
}
