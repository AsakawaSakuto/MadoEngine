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

Animation LoadAnimationFile(const std::string& filename, int index = 0);

Skeleton CreateSkeleton(const ModelNode& rootNode);

SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData, uint32_t srvIndex);

int32_t CreateJoint(const ModelNode& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);

Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);

void UpdateAnimation(Skeleton& skeleton);

void UpdateCluster(SkinCluster& skinCluster, const Skeleton& skeleton);

void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);