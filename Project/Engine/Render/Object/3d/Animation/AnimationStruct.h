#pragma once
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Math/Transform.h"
#include "Math/Matrix4x4.h"
#include <vector>
#include <map>
#include <cassert> 
#include <optional>
#include <array>
#include <span>
#include <d3d12.h>
#include <wrl/client.h>
#include <string>

template <typename tValue>
struct Keyframe {
    float time;
    tValue value;
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template<typename tValue>
struct AnimationCurve {
    std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
    AnimationCurve<Vector3> translate;
    AnimationCurve<Quaternion> rotate;
    AnimationCurve<Vector3> scale;
};

struct Animation {
    float duration;  // アニメーション全体の尺（単位は秒）
    // NodeAnimationの集合。Node名でひけるようにしておく
    std::map<std::string, NodeAnimation> nodeAnimations;
};

struct Joint {
    QuaternionTransform transform; // Transform情報
    Matrix4x4 localMatrix;         // localMatrix
    Matrix4x4 skeletonSpaceMatrix; // skeletonSpaceでの変換行列
    std::string name;              // 名前
    std::vector<int32_t> children; // 子JointのIndexのリスト。いなければ空
    int32_t index;                 // 自身のIndex
    std::optional<int32_t> parent; // 親JointのIndex。いなければnull
};

struct Skeleton {
    int32_t root;                            // RootJointのIndex
    std::map<std::string, int32_t> jointMap; // Joint名とIndexとの辞書
    std::vector<Joint> joints;               // 所属しているジョイント
};

const uint32_t kNumMaxInfluence = 4;

struct VertexInfluence {
    std::array<float, kNumMaxInfluence> weights;
    std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU {
    Matrix4x4 skeletonSpaceMatrix;                 // 位置用
    Matrix4x4 skeletonSpaceInverseTransposeMatrix; // 法線用
};

struct SkinCluster {
    std::vector<Matrix4x4> inverseBindPoseMatrices;
    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
    D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
    std::span<VertexInfluence> mappedInfluence;
    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
    std::span<WellForGPU> mappedPalette;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};