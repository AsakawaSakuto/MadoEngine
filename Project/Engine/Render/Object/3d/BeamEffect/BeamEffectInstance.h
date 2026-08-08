#pragma once
#include "BeamEffectAsset.h"
#include "BeamPointGenerator.h"
#include <memory>
#include <vector>

namespace MadoEngine::Ribbon {
	class RibbonEffectRenderer3d;
}

namespace MadoEngine::Beam {

	/// @brief 1つのBeam再生状態と追従する始点、終点を管理する
	class BeamEffectInstance final {
	public:
		/// @brief 再生状態を初期化する
		/// @param asset 再生するBeam Asset
		/// @param desc 再生設定
		void Initialize(
			std::shared_ptr<const BeamEffectAsset> asset,
			const BeamEffectPlayDesc& desc
		);

		/// @brief 再生時間を更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Beamの再生を停止する
		/// @param mode 停止方式
		void Stop(BeamStopMode mode);

		/// @brief Beam Effectの時間進行を一時停止する
		void Pause();

		/// @brief Beam Effectの時間進行を再開する
		void Resume();

		/// @brief Beam Effectの再生速度を設定する
		/// @param playbackSpeed 設定する再生速度
		/// @return 有効な再生速度を設定できた場合はtrue
		bool SetPlaybackSpeed(float playbackSpeed);

		/// @brief Beam Effectが一時停止中か確認する
		/// @return 一時停止中の場合はtrue
		bool IsPaused() const {
			return isPaused_;
		}

		/// @brief Instanceが終了済みか確認する
		/// @return 終了済みの場合はtrue
		bool IsFinished() const;

		/// @brief 描画条件に一致するか確認する
		/// @param sceneType 現在Scene
		/// @param layerMask 描画対象Layer Mask
		/// @return 条件に一致する場合はtrue
		bool Matches(SceneType sceneType, MadoEngine::Render::RenderLayerMask layerMask) const;

		/// @brief Beam描画データをRibbon Rendererへ登録する
		/// @param renderer 内部描画に使うRibbon Renderer
		void SubmitRenderData(MadoEngine::Ribbon::RibbonEffectRenderer3d& renderer) const;

		/// @brief 始点と終点を更新する
		/// @param startPosition 新しい始点
		/// @param endPosition 新しい終点
		void SetEndpoints(const Vector3& startPosition, const Vector3& endPosition);

		/// @brief 始点を更新する
		/// @param position 新しい始点
		void SetStartPosition(const Vector3& position);

		/// @brief 終点を更新する
		/// @param position 新しい終点
		void SetEndPosition(const Vector3& position);

		/// @brief Instanceが属するSceneを取得する
		/// @return 所属Scene
		SceneType GetSceneType() const {
			return sceneType_;
		}

	private:
		/// @brief 1つのEmitterに固有な再生状態
		struct EmitterState {
			BeamEmitterConfig config;
			float playbackTime = 0.0f;
			float totalTime = 0.0f;
			bool isLoop = false;
			bool isStopping = false;
			bool isFinished = false;
		};

		std::shared_ptr<const BeamEffectAsset> asset_;
		std::vector<EmitterState> emitters_;
		BeamPointGenerator pointGenerator_;
		Vector3 startPosition_{};
		Vector3 endPosition_{};
		SceneType sceneType_ = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Effect;
		float playbackSpeed_ = 1.0f;
		bool isPaused_ = false;
	};

} // namespace MadoEngine::Beam
