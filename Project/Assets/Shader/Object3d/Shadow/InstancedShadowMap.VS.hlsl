#include "../Model/Model.hlsli"

StructuredBuffer<InstanceData> gInstanceData : register(t2);

/// @brief InstanceごとのLight空間行列でShadow Map用頂点を変換
/// @param input Modelの頂点情報
/// @param instanceId 描画対象のInstance番号
/// @return Light Clip空間の頂点位置
float4 main(VertexShaderInput input, uint instanceId : SV_InstanceID) : SV_POSITION
{

    // Instance WVPはShadow描画時にLight ViewProjectionを含む行列としてCPU側から転送
    InstanceData instance = gInstanceData[instanceId];
    return mul(input.position, instance.WVP);
}
