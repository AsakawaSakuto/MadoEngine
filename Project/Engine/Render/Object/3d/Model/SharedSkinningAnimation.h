#pragma once
#include "ModelSharedData.h"
#include <array>
#include <cstdint>
#include <limits>

namespace MadoEngine {

/// @brief 同じスキニングモデルのループ再生時刻と位相ごとのパレットを管理
class SharedSkinningAnimation {
public:
	static constexpr std::uint32_t kPhaseGroupCount = 8;

	/// @brief モデルの骨格と共有するループクリップを設定
	/// @param modelData 共有するモデルデータ
	/// @param clip 再生するループクリップ
	SharedSkinningAnimation(const ModelData& modelData, const AnimationClip& clip);

	/// @brief 位相グループが所有するSRVの解放
	~SharedSkinningAnimation();

	SharedSkinningAnimation(const SharedSkinningAnimation&) = delete;
	SharedSkinningAnimation& operator=(const SharedSkinningAnimation&) = delete;

	/// @brief 位相グループの初回利用時に描画リソースを確保
	/// @param device D3D12デバイス
	/// @param phaseGroup 確保する位相グループ番号
	void PrepareGroup(ID3D12Device* device, std::uint32_t phaseGroup);

	/// @brief 全位相グループが参照する時計を更新
	/// @param deltaTime 前フレームからの再生時間
	void AdvanceTime(float deltaTime);

	/// @brief 現在の再生時刻に対応する位相グループの描画データを取得
	/// @param phaseGroup 取得する位相グループ番号
	/// @return ボーンパレットと頂点ウェイトを保持する描画データ
	const SkinCluster& GetSkinCluster(std::uint32_t phaseGroup);

	/// @brief 共有再生中のクリップ名を取得
	/// @return 共有再生中のクリップ名
	const std::string& GetClipName() const { return clip_.name; }

private:
	struct PhaseGroup {
		SkinCluster skinCluster{};
		std::uint32_t descriptorIndex = (std::numeric_limits<std::uint32_t>::max)();
		double sampledPlaybackTime = -1.0;
	};

	const ModelData& modelData_;
	const AnimationClip& clip_;
	Skeleton skeleton_;
	double playbackTime_ = 0.0;
	std::array<PhaseGroup, kPhaseGroupCount> phaseGroups_{};
};

} // namespace MadoEngine
