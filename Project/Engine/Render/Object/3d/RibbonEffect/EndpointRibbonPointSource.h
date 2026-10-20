#pragma once
#include "IRibbonPointSource.h"

namespace MadoEngine::Ribbon {

	/// @brief 始点と終点を分割した固定形状の点生成Strategy
	class EndpointRibbonPointSource final : public IRibbonPointSource {
	public:
		/// @brief 始終点方式の点生成を初期化
		/// @param config 始終点と分割数
		/// @param transform 始終点の基準Transform
		EndpointRibbonPointSource(const RibbonEndpointModule& config, const Transform3D& transform);

		/// @brief 固定形状の再生状態を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief AssetのLocal始終点へ基準Transformを反映
		/// @param transform 新しい基準Transform
		void SetTransform(const Transform3D& transform) override;

		/// @brief World始終点を設定
		/// @param endpoints 新しいWorld始終点
		/// @return 再生中に有限な始終点を設定できた場合はtrue
		bool SetEndpoints(const RibbonEndpoints& endpoints) override;

		/// @brief 固定形状の再生を終了
		/// @param mode 停止方式
		void Stop(RibbonStopMode mode) override;

		/// @brief 点列を破棄
		void Clear() override;

		/// @brief 始点から終点へ並んだWorld点列を取得
		/// @return World点列
		const std::vector<RibbonPoint>& GetPoints() const override { return points_; }

	private:
		/// @brief 現在の始終点から点列を再構築
		void RebuildPoints();

		RibbonEndpointModule config_;
		RibbonEndpoints worldEndpoints_;
		std::vector<RibbonPoint> points_;
		bool isGenerating_ = true;
		bool hasWorldOverride_ = false;
	};

} // namespace MadoEngine::Ribbon
