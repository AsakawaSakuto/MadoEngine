#pragma once
#include "CylinderEffectAsset.h"
#include <cstddef>
#include <memory>

namespace MadoEngine::Effect {

	class CylinderEffectRenderer3d;

	/// @brief Cylinder Effect Assetの1回分の再生状態
	class CylinderEffectInstance final {
	public:
		/// @brief 再生状態を初期化する
		/// @param asset 再生するCylinder Effect Asset
		/// @param desc 再生設定
		void Initialize(std::shared_ptr<const CylinderEffectAsset> asset, const PrimitiveEffectPlayDesc& desc);

		/// @brief 再生時刻を更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime);

		/// @brief エフェクトを停止する
		/// @param mode 停止方法
		void Stop(PrimitiveEffectStopMode mode);

		/// @brief 再生が終了したか確認する
		/// @return 再生終了済みの場合はtrue
		bool IsFinished() const;

		/// @brief 描画条件に一致するか確認する
		/// @param sceneType 描画対象Scene
		/// @param layerMask 描画対象LayerMask
		/// @return 描画条件に一致する場合はtrue
		bool Matches(SceneType sceneType, MadoEngine::Render::RenderLayerMask layerMask) const;

		/// @brief 現在値を評価してRendererへ登録する
		/// @param renderer 登録先Renderer
		void SubmitRenderData(CylinderEffectRenderer3d& renderer) const;

		/// @brief Transformを設定する
		/// @param transform 設定するTransform
		void SetTransform(const Transform3D& transform) {
			transform_ = transform;
		}

		/// @brief Transformを取得する
		/// @return 現在のTransform
		const Transform3D& GetTransform() const {
			return transform_;
		}

		/// @brief 所属Sceneを取得する
		/// @return 所属Scene
		SceneType GetSceneType() const {
			return sceneType_;
		}

	private:
		std::shared_ptr<const CylinderEffectAsset> asset_;
		Transform3D transform_;
		SceneType sceneType_ = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Effect;
		float playbackTime_ = 0.0f;
		bool isLoop_ = false;
		bool isFinished_ = false;
	};

} // namespace MadoEngine::Effect
