#pragma once
#include "IRibbonPointSource.h"
#include "RibbonEffectAsset.h"
#include <memory>
#include <vector>

namespace MadoEngine::Ribbon {

	class RibbonEffectRenderer3d;

	/// @brief Ribbon Effect Assetの1回分の再生状態を管理する
	class RibbonEffectInstance final {
	public:
		/// @brief 再生状態を初期化する
		/// @param asset 再生するRibbon Asset
		/// @param desc 再生設定
		void Initialize(
			std::shared_ptr<const RibbonEffectAsset> asset,
			const RibbonEffectPlayDesc& desc
		);

		/// @brief 再生時間とPoint寿命を更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief Ribbonの再生を停止する
		/// @param mode 停止方式
		void Stop(RibbonStopMode mode);

		/// @brief Ribbon Effectの時間進行と履歴生成を一時停止する
		void Pause();

		/// @brief Ribbon Effectの時間進行と履歴生成を再開する
		void Resume();

		/// @brief Ribbon Effectの再生速度を設定する
		/// @param playbackSpeed 設定する再生速度
		/// @return 有効な再生速度を設定できた場合はtrue
		bool SetPlaybackSpeed(float playbackSpeed);

		/// @brief Ribbon Effectが一時停止中か確認する
		/// @return 一時停止中の場合はtrue
		bool IsPaused() const {
			return isPaused_;
		}

		/// @brief Instanceが終了したか確認する
		/// @return 終了済みの場合はtrue
		bool IsFinished() const;

		/// @brief 描画条件に一致するか確認する
		/// @param sceneType 描画対象Scene
		/// @param layerMask 描画対象Layer Mask
		/// @return 条件に一致する場合はtrue
		bool Matches(
			SceneType sceneType,
			MadoEngine::Render::RenderLayerMask layerMask
		) const;

		/// @brief 現在のPointとAsset設定から描画データを登録する
		/// @param renderer 登録先Renderer
		void SubmitRenderData(RibbonEffectRenderer3d& renderer) const;

		/// @brief 追跡対象Transformを更新する
		/// @param transform 最新Transform
		void SetTransform(const Transform3D& transform);

		/// @brief Manual Ribbonの制御点を置き換える
		/// @param controlPoints 設定順に並んだ制御点
		/// @return Manual Sourceへ設定できた場合はtrue
		bool SetControlPoints(const std::vector<Vector3>& controlPoints);

		/// @brief Manual Ribbonの制御点を破棄する
		/// @return Manual Sourceを消去できた場合はtrue
		bool ClearControlPoints();

		/// @brief Instanceが所属するSceneを取得する
		/// @return 所属Scene
		SceneType GetSceneType() const {
			return sceneType_;
		}

	private:
		std::shared_ptr<const RibbonEffectAsset> asset_;
		std::unique_ptr<IRibbonPointSource> pointSource_;
		Transform3D transform_;
		SceneType sceneType_ = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Effect;
		float playbackTime_ = 0.0f;
		float totalTime_ = 0.0f;
		float playbackSpeed_ = 1.0f;
		bool isLoop_ = false;
		bool isGenerating_ = false;
		bool isImmediatelyFinished_ = false;
		bool isPaused_ = false;
	};

} // namespace MadoEngine::Ribbon
