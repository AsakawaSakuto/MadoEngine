#pragma once
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "../Model/ModelData.h"
#include "AnimationStruct.h"
#include <vector>
#include <cassert> 
#include <d3d12.h>
#include <wrl/client.h>

namespace MadoEngine::Core { class SRVManager; }

/// @brief ファイルから指定IndexのAnimationClipを読み込み
/// @param filename 読み込むAnimationファイルパス
/// @param index 読み込むAnimation Index
/// @return 読み込んだAnimationClip
AnimationClip LoadAnimationFile(const std::string& filename, int index = 0);

Skeleton CreateSkeleton(const ModelNode& rootNode);

SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData, uint32_t srvIndex);

int32_t CreateJoint(const ModelNode& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);

Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);

void UpdateAnimation(Skeleton& skeleton);

void UpdateCluster(SkinCluster& skinCluster, const Skeleton& skeleton);

/// @brief AnimationClipをSkeletonへ直接反映
/// @param skeleton 反映対象のSkeleton
/// @param animation 反映するAnimationClip
/// @param animationTime 評価する再生時刻
void ApplyAnimation(Skeleton& skeleton, const AnimationClip& animation, float animationTime);

/// @brief SkeletonのBindPoseをAnimationPoseへ展開
/// @param skeleton Pose生成の基準になるSkeleton
/// @param outPose 生成したPoseの出力先
void CreateBindPose(const Skeleton& skeleton, AnimationPose& outPose);

/// @brief AnimationClipを指定時刻で評価
/// @param skeleton Pose生成の基準になるSkeleton
/// @param animation 評価するAnimationClip
/// @param animationTime 評価する再生時刻
/// @param outPose 評価したPoseの出力先
void SampleAnimationPose(const Skeleton& skeleton, const AnimationClip& animation, float animationTime, AnimationPose& outPose);

/// @brief 二つのAnimationPoseを補間
/// @param sourcePose 遷移元Pose
/// @param targetPose 遷移先Pose
/// @param blendFactor 補間係数
/// @param outPose 補間結果の出力先
void BlendAnimationPose(const AnimationPose& sourcePose, const AnimationPose& targetPose, float blendFactor, AnimationPose& outPose);

/// @brief AnimationPoseをSkeletonへ反映
/// @param skeleton 反映対象のSkeleton
/// @param pose 反映するAnimationPose
void ApplyAnimationPose(Skeleton& skeleton, const AnimationPose& pose);
