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
};

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
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
