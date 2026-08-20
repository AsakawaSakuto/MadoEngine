#pragma once
#include "EffectSequenceAsset.h"
#include "EffectSequenceNodeDispatcher.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace MadoEngine::EffectSequence {

	/// @brief Effect Sequence Assetの1回分の再生状態を管理する
	class EffectSequenceInstance final {
	public:
		/// @brief Sequence Instanceを初期化
		/// @param asset 再生するSequence Asset
		/// @param desc 再生設定
		/// @param dispatcher 子Effectへの指示を担当するDispatcher
		void Initialize(
			std::shared_ptr<const EffectSequenceAsset> asset,
			const EffectSequencePlayDesc& desc,
			const EffectSequenceNodeDispatcher& dispatcher
		);

		/// @brief Sequence再生時間と子Effect状態を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Sequenceを停止
		/// @param mode 停止方式
		void Stop(EffectSequenceStopMode mode);

		/// @brief Scene切り替えまたはSystem終了により即時破棄
		/// @param reason 破棄理由
		void Destroy(EffectSequenceFinishReason reason);

		/// @brief Sequenceと再生中の子Effectを一時停止
		void Pause();

		/// @brief Sequenceと再生中の子Effectを再開
		void Resume();

		/// @brief Sequence Root Transformを更新
		/// @param transform 新しいRoot Transform
		void SetTransform(const Transform3D& transform);

		/// @brief Sequenceと再生中の子Effectの表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief 再生中のPrimitive Effectへ色倍率を設定
		/// @param colorMultiplier Assetの色へ乗算する色倍率
		/// @return 有効な色倍率を設定できた場合はtrue
		bool SetColorMultiplier(const Vector4& colorMultiplier);

		/// @brief Sequence再生速度を設定
		/// @param playbackSpeed 再生速度
		/// @return 有効な再生速度を設定できた場合はtrue
		bool SetPlaybackSpeed(float playbackSpeed);

		/// @brief Sequenceが終了したか確認
		/// @return 終了済みの場合はtrue
		bool IsFinished() const {
			return isFinished_;
		}

		/// @brief Sequenceが一時停止中か確認
		/// @return 一時停止中の場合はtrue
		bool IsPaused() const {
			return isPaused_;
		}

		/// @brief Sequenceの現在再生時間を取得
		/// @return 現在再生時間
		float GetPlaybackTime() const {
			return playbackTime_;
		}

		/// @brief SequenceのAsset名を取得
		/// @return Asset名
		const std::string& GetAssetName() const;

		/// @brief Sequenceが所属するSceneを取得
		/// @return 所属Scene
		SceneType GetSceneType() const {
			return sceneType_;
		}

		/// @brief Sequenceの終了理由を取得
		/// @return 終了理由
		EffectSequenceFinishReason GetFinishReason() const {
			return finishReason_;
		}

		/// @brief Sequenceの再生用途を取得
		/// @return 再生用途
		EffectSequencePlaybackContext GetPlaybackContext() const {
			return context_;
		}

	private:
		struct ActiveChild {
			uint32_t nodeId = 0;
			EffectSequenceChildHandle handle;
		};

		/// @brief 指定時間区間に開始するNodeを発火
		/// @param previousTime 区間開始時刻
		/// @param currentTime 区間終了時刻
		/// @param includePrevious 区間開始時刻を含める場合はtrue
		void FireNodes(float previousTime, float currentTime, bool includePrevious);

		/// @brief 指定Nodeを子Effect Systemへ発火
		/// @param node 発火するNode
		void FireNode(const EffectSequenceNode& node);

		/// @brief Node階層のWorld Transformを再計算
		void RebuildWorldTransforms();

		/// @brief NodeのWorld Transformを循環防止付きで計算
		/// @param nodeIndex 計算するNode Index
		/// @param visitStates 探索状態
		/// @return 計算したWorld Transform
		Transform3D ResolveWorldTransform(std::size_t nodeIndex, std::vector<uint8_t>& visitStates);

		/// @brief 再生中の子EffectへWorld Transformを反映
		void ApplyWorldTransformsToChildren();

		/// @brief 再生中の子Effectへ再生速度を反映
		void ApplyPlaybackSpeedToChildren();

		/// @brief 再生中の子Effectを停止
		/// @param mode 停止方式
		void StopChildren(EffectSequenceStopMode mode);

		/// @brief 終了済み子Effectを管理対象から除外
		void RemoveFinishedChildren();

		/// @brief 次のLoop周回を開始
		void BeginNextLoop();

		/// @brief 子Effectがすべて終了した場合にSequenceを終了
		void TryFinishWaitingSequence();

		std::shared_ptr<const EffectSequenceAsset> asset_;
		const EffectSequenceNodeDispatcher* dispatcher_ = nullptr;
		std::vector<ActiveChild> activeChildren_;
		std::unordered_set<uint32_t> firedNodeIds_;
		std::unordered_map<uint32_t, std::size_t> nodeIndices_;
		std::unordered_map<uint32_t, Transform3D> worldTransforms_;
		Transform3D rootTransform_;
		Vector4 colorMultiplier_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		SceneType sceneType_ = SceneType::None;
		MadoEngine::Render::RenderLayer defaultRenderLayer_ = MadoEngine::Render::RenderLayer::Effect;
		EffectSequencePlaybackContext context_ = EffectSequencePlaybackContext::Game;
		EffectSequenceFinishReason finishReason_ = EffectSequenceFinishReason::Natural;
		float playbackTime_ = 0.0f;
		float playbackSpeed_ = 1.0f;
		bool isLoop_ = false;
		bool isPaused_ = false;
		bool isVisible_ = true;
		bool isWaitingForChildren_ = false;
		bool isFinished_ = true;
	};

} // namespace MadoEngine::EffectSequence
