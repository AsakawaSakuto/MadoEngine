#pragma once
#include "IRibbonPointSource.h"
#include "Math/Matrix4x4.h"

namespace MadoEngine::Ribbon {

	/// @brief Transformの移動履歴からRibbon Pointを生成するStrategy
	class TrailPointSource final : public IRibbonPointSource {
	public:
		/// @brief Trail Point Sourceを構築
		/// @param config Trail生成設定
		/// @param initialTransform 再生開始時Transform
		TrailPointSource(const RibbonTrailModule& config, const Transform3D& initialTransform);

		/// @brief Pointの経過時間と寿命を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief Transform位置から必要に応じてPointを生成
		/// @param transform 最新Transform
		void SetTransform(const Transform3D& transform) override;

		/// @brief 新規Point生成を停止
		/// @param mode 停止方式
		void Stop(RibbonStopMode mode) override;

		/// @brief 保持しているPointをすべて破棄
		void Clear() override;

		/// @brief World座標へ解決済みのPoint列を取得
		/// @return 古い順に並んだPoint列
		const std::vector<RibbonPoint>& GetPoints() const override {
			return worldPoints_;
		}

	private:
		/// @brief 距離条件を満たすPointを追加
		/// @param worldPosition 追加候補のWorld座標
		void TryAddPoint(const Vector3& worldPosition);

		/// @brief Local Point列からWorld Point列を再構築
		void RebuildWorldPoints();

		RibbonTrailModule config_;
		Matrix4x4 localToWorld_{};
		Matrix4x4 worldToLocal_{};
		std::vector<RibbonPoint> sourcePoints_;
		std::vector<RibbonPoint> worldPoints_;
		bool isGenerating_ = true;
	};

} // namespace MadoEngine::Ribbon
