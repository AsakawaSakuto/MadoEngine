#include "Model.hlsli"

StructuredBuffer<InstanceData> gInstanceData : register(t2);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    InstanceData instance = gInstanceData[instanceId];

    VertexShaderOutput output;
    output.position = mul(input.position, instance.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) instance.WorldInverseTranspose));
    output.worldPosition = mul(input.position, instance.World).xyz;
    output.color = instance.color;

    return output;
}
