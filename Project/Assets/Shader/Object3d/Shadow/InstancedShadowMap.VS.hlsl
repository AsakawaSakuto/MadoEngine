#include "../Model/Model.hlsli"

StructuredBuffer<InstanceData> gInstanceData : register(t2);

float4 main(VertexShaderInput input, uint instanceId : SV_InstanceID) : SV_POSITION
{
    InstanceData instance = gInstanceData[instanceId];
    return mul(input.position, instance.WVP);
}
