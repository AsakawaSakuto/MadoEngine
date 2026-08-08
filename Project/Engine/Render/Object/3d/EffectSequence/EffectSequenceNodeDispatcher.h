#pragma once
#include "EffectSequenceTypes.h"
#include <optional>

namespace MadoEngine::EffectSequence {

	/// @brief Sequence Nodeと各Effect SystemのAPI差分を変換する
	class EffectSequenceNodeDispatcher final {
	public:
		/// @brief Nodeに対応する子Effectを再生する
		/// @param node 再生するNode
		/// @param worldTransform NodeのWorld Transform
		/// @param sceneType 所属Scene
		/// @param defaultRenderLayer Sequence既定RenderLayer
		/// @param playbackSpeed 子Effectへ適用する再生速度
		/// @return 再生に成功した子Handle。失敗時はstd::nullopt
		std::optional<EffectSequenceChildHandle> Play(
			const EffectSequenceNode& node,
			const Transform3D& worldTransform,
			SceneType sceneType,
			MadoEngine::Render::RenderLayer defaultRenderLayer,
			float playbackSpeed
		) const;

		/// @brief 子Effectを停止する
		/// @param handle 停止する子Handle
		/// @param mode 停止方式
		void Stop(const EffectSequenceChildHandle& handle, EffectSequenceStopMode mode) const;

		/// @brief 子Effectを一時停止する
		/// @param handle 一時停止する子Handle
		void Pause(const EffectSequenceChildHandle& handle) const;

		/// @brief 子Effectを再開する
		/// @param handle 再開する子Handle
		void Resume(const EffectSequenceChildHandle& handle) const;

		/// @brief 子EffectのTransformをNode種別に応じて更新する
		/// @param node 子Effectを生成したNode
		/// @param handle 更新する子Handle
		/// @param worldTransform NodeのWorld Transform
		void SetTransform(
			const EffectSequenceNode& node,
			const EffectSequenceChildHandle& handle,
			const Transform3D& worldTransform
		) const;

		/// @brief 子Effectの再生速度を設定する
		/// @param handle 設定する子Handle
		/// @param playbackSpeed 再生速度
		void SetPlaybackSpeed(
			const EffectSequenceChildHandle& handle,
			float playbackSpeed
		) const;

		/// @brief 子Effectが再生中か確認する
		/// @param handle 確認する子Handle
		/// @return 再生中の場合はtrue
		bool IsAlive(const EffectSequenceChildHandle& handle) const;
	};

} // namespace MadoEngine::EffectSequence
