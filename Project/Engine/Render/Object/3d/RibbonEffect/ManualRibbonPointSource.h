#pragma once
#include "IRibbonPointSource.h"

namespace MadoEngine::Ribbon {

	/// @brief ゲーム側から設定された制御点を使用するRibbon Point Strategy
	class ManualRibbonPointSource final : public IRibbonPointSource {
	public:
		/// @brief Manual Point Sourceを構築する
		/// @param config Point保持設定
		/// @param initialTransform 初期Transform
		ManualRibbonPointSource(const RibbonTrailModule& config, const Transform3D& initialTransform);

		/// @brief 停止後のPoint寿命を更新する
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Local空間制御点へ適用するTransformを更新する
		/// @param transform 最新Transform
		void SetTransform(const Transform3D& transform) override;

		/// @brief 制御点の維持を停止する
		/// @param mode 停止方式
		void Stop(RibbonStopMode mode) override;

		/// @brief 保持している制御点をすべて破棄する
		void Clear() override;

		/// @brief World座標へ解決済みのPoint列を取得する
		/// @return 設定順に並んだPoint列
		const std::vector<RibbonPoint>& GetPoints() const override {
			return worldPoints_;
		}

		/// @brief ゲーム側制御点を置き換える
		/// @param controlPoints 設定順に並んだ制御点
		/// @return 有効な制御点を設定できた場合はtrue
		bool SetControlPoints(const std::vector<Vector3>& controlPoints);

	private:
		/// @brief 制御点をWorld座標へ変換して描画Pointを再構築する
		void RebuildWorldPoints();

		RibbonTrailModule config_;
		Transform3D transform_;
		std::vector<RibbonPoint> sourcePoints_;
		std::vector<RibbonPoint> worldPoints_;
		bool isGenerating_ = true;
	};

} // namespace MadoEngine::Ribbon
