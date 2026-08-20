#pragma once
#include "CylinderEffectAsset.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace MadoEngine::Effect {

	class CylinderEffectRenderer3d;

	/// @brief Cylinder Effect Assetの1回分の再生状態
	class CylinderEffectInstance final {
	public:
		/// @brief 再生状態を初期化
		/// @param asset 再生するCylinder Effect Asset
		/// @param desc 再生設定
		void Initialize(std::shared_ptr<const CylinderEffectAsset> asset, const PrimitiveEffectPlayDesc& desc);

		/// @brief 再生時刻を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief エフェクトを停止
		/// @param mode 停止方法
		void Stop(PrimitiveEffectStopMode mode);

		/// @brief Cylinder Effectの時間進行を一時停止
		void Pause();

		/// @brief Cylinder Effectの時間進行を再開
		void Resume();

		/// @brief Cylinder Effectの再生速度を設定
		/// @param playbackSpeed 設定する再生速度
		/// @return 有効な再生速度を設定できた場合はtrue
		bool SetPlaybackSpeed(float playbackSpeed);

		/// @brief Assetの色へ乗算する色倍率を設定
		/// @param colorMultiplier 色倍率
		/// @return 有効な色倍率を設定できた場合はtrue
		bool SetColorMultiplier(const Vector4& colorMultiplier);

		/// @brief Cylinder Effectが一時停止中か確認
		/// @return 一時停止中の場合はtrue
		bool IsPaused() const {
			return isPaused_;
		}

		/// @brief 再生が終了したか確認
		/// @return 再生終了済みの場合はtrue
		bool IsFinished() const;

		/// @brief 描画条件に一致するか確認
		/// @param sceneType 描画対象Scene
		/// @param layerMask 描画対象LayerMask
		/// @return 描画条件に一致する場合はtrue
		bool Matches(SceneType sceneType, MadoEngine::Render::RenderLayerMask layerMask) const;

		/// @brief 現在値を評価してRendererへ登録
		/// @param renderer 登録先Renderer
		void SubmitRenderData(CylinderEffectRenderer3d& renderer) const;

		/// @brief Transformを設定
		/// @param transform 設定するTransform
		void SetTransform(const Transform3D& transform) {
			transform_ = transform;
		}

		/// @brief Transformを取得
		/// @return 現在のTransform
		const Transform3D& GetTransform() const {
			return transform_;
		}

		/// @brief 所属Sceneを取得
		/// @return 所属Scene
		SceneType GetSceneType() const {
			return sceneType_;
		}

	private:
		/// @brief 1つのEmitterに固有な再生状態
		struct EmitterState {
			CylinderEmitterConfig config;
			float playbackTime = 0.0f;
			bool isLoop = false;
			bool isFinished = false;
		};

		std::shared_ptr<const CylinderEffectAsset> asset_;
		std::vector<EmitterState> emitters_;
		Transform3D transform_;
		SceneType sceneType_ = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Effect;
		Vector4 colorMultiplier_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		float playbackSpeed_ = 1.0f;
		bool isPaused_ = false;
	};

} // namespace MadoEngine::Effect
