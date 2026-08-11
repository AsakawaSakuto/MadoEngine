#include "CopyImage.hlsli"

static const uint kNumVertex = 3;

// 画面対角の補間継ぎ目を避けるため一枚の大型TriangleでViewport全体を被覆
static const float4 kPositions[kNumVertex] =
{
    float4(-1.0f, 1.0f, 0.0f, 1.0f),
    float4(3.0f, 1.0f, 0.0f, 1.0f),
    float4(-1.0f, -3.0f, 0.0f, 1.0f),
};

// Viewport外まで伸ばしたClip座標に対応させる0から2のUV
static const float2 kTexcoords[kNumVertex] =
{
    float2(0.0f, 0.0f),
    float2(2.0f, 0.0f),
    float2(0.0f, 2.0f),
};

/// @brief SV_VertexIDからFullscreen Triangleを生成
/// @param vertexId Triangle内の頂点番号
/// @return Clip座標とTexture UV
VertexShaderOutput main(uint vertexId : SV_VertexID)
{
    VertexShaderOutput output;
    output.position = kPositions[vertexId];
    output.texcoord = kTexcoords[vertexId];
    return output;
}
