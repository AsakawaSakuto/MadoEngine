struct CylinderInstance
{
    row_major float4x4 world;
    float4 radii;
    float4 geometry;
    float4 uvTransform;
    float4 effectParameters;
    float4 gradientColors[8];
    float4 gradientPositions[2];
    uint4 metadata;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 parameter : TEXCOORD0;
    nointerpolation uint instanceId : TEXCOORD1;
};

StructuredBuffer<CylinderInstance> gInstances : register(t0);

cbuffer PerView : register(b0)
{
    row_major float4x4 gViewProjection;
};

/// @brief 正規化グリッドからCylinder側面の頂点を生成する
/// @param vertexId インデックスバッファから渡される頂点番号
/// @param instanceId 描画するCylinder Instance番号
/// @return 変換済み頂点
VertexShaderOutput main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    CylinderInstance instance = gInstances[instanceId];
    const uint radialSegments = max(instance.metadata.x, 3u);
    const uint heightSegments = max(instance.metadata.y, 1u);
    const uint columnCount = radialSegments + 1u;
    const uint radialIndex = vertexId % columnCount;
    const uint heightIndex = vertexId / columnCount;
    const float u = (float)radialIndex / (float)radialSegments;
    const float v = (float)heightIndex / (float)heightSegments;

    const float angle = instance.geometry.y + instance.geometry.z * u;
    float sine = 0.0f;
    float cosine = 1.0f;
    sincos(angle, sine, cosine);

    const float2 radii = lerp(instance.radii.xy, instance.radii.zw, v);
    float pivotOffset = 0.0f;
    const uint pivot = (uint)round(instance.effectParameters.w);
    if (pivot == 1u)
    {
        pivotOffset = -0.5f;
    }
    else if (pivot == 2u)
    {
        pivotOffset = -1.0f;
    }

    const float3 localPosition = float3(
        -sine * radii.x,
        (v + pivotOffset) * instance.geometry.x,
        cosine * radii.y
    );
    const float4 worldPosition = mul(float4(localPosition, 1.0f), instance.world);

    VertexShaderOutput output;
    output.position = mul(worldPosition, gViewProjection);
    output.parameter = float2(u, v);
    output.instanceId = instanceId;
    return output;
}
