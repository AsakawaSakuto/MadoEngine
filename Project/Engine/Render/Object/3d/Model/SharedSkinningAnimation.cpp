#include "SharedSkinningAnimation.h"
#include "../Animation/AnimationFunction.h"
#include "Core/View/SRVManager.h"
#include <cassert>
#include <cmath>

namespace MadoEngine {

SharedSkinningAnimation::SharedSkinningAnimation(const ModelData& modelData, const AnimationClip& clip)
	: modelData_(modelData), clip_(clip), skeleton_(CreateSkeleton(modelData.rootNode)) {
}

SharedSkinningAnimation::~SharedSkinningAnimation() {
	for (const PhaseGroup& group : phaseGroups_) {
		if (group.descriptorIndex != (std::numeric_limits<std::uint32_t>::max)()) {
			Core::SRVManager::GetInstance().Free(group.descriptorIndex);
		}
	}
}

void SharedSkinningAnimation::PrepareGroup(ID3D12Device* device, std::uint32_t phaseGroup) {
	assert(phaseGroup < phaseGroups_.size());
	PhaseGroup& group = phaseGroups_[phaseGroup];

	// 同じ位相の敵が増えてもGPUリソースを再確保しないため初回だけ初期化
	if (group.descriptorIndex == (std::numeric_limits<std::uint32_t>::max)()) {
		Core::SRVManager& srvManager = Core::SRVManager::GetInstance();
		const std::uint32_t descriptorIndex = srvManager.Allocate();
		try {
			group.skinCluster = CreateSkinCluster(device, skeleton_, modelData_, descriptorIndex);
		} catch (...) {

			// 生成失敗後の再試行で未初期化のパレットを再利用しないようSRVを返却
			srvManager.Free(descriptorIndex);
			throw;
		}
		group.descriptorIndex = descriptorIndex;
	}
}

void SharedSkinningAnimation::AdvanceTime(float deltaTime) {
	if (std::isfinite(deltaTime) && deltaTime > 0.0f && clip_.playbackSpeed > 0.0f) {

		// 長時間のプレイでも位相の精度を維持するため時計を一周の範囲に制限
		playbackTime_ = std::fmod(playbackTime_ + static_cast<double>(deltaTime) * clip_.playbackSpeed, clip_.duration);

		// 周期内でグループの更新タイミングをずらし、骨格計算を分散
		frameInUpdateCycle_ = (frameInUpdateCycle_ + 1) % kPoseUpdateInterval;
		for (std::uint32_t phaseGroup = 0; phaseGroup < kPhaseGroupCount; ++phaseGroup) {
			const std::uint32_t updateFrame = phaseGroup * kPoseUpdateInterval / kPhaseGroupCount;
			if (frameInUpdateCycle_ == updateFrame) {
				phaseGroups_[phaseGroup].needsPoseUpdate = true;
			}
		}
	}
}

const SkinCluster& SharedSkinningAnimation::GetSkinCluster(std::uint32_t phaseGroup) {
	assert(phaseGroup < phaseGroups_.size());
	PhaseGroup& group = phaseGroups_[phaseGroup];
	assert(group.descriptorIndex != (std::numeric_limits<std::uint32_t>::max)());

	// 初回と更新待ちのグループだけ現在時刻のポーズを計算し、通常描画と影描画で共有
	if (group.needsPoseUpdate) {
		const double phaseOffset = static_cast<double>(clip_.duration) * phaseGroup / kPhaseGroupCount;
		const float sampleTime = static_cast<float>(std::fmod(playbackTime_ + phaseOffset, clip_.duration));
		ApplyAnimation(skeleton_, clip_, sampleTime);
		UpdateAnimation(skeleton_);
		UpdateCluster(group.skinCluster, skeleton_);
		group.needsPoseUpdate = false;
	}

	return group.skinCluster;
}

} // namespace MadoEngine
