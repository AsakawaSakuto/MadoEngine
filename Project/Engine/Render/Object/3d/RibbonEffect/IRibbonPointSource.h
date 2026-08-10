#pragma once
#include "RibbonEffectTypes.h"
#include <vector>

namespace MadoEngine::Ribbon {

	/// @brief Ribbon制御点を生成し寿命管理するStrategy Interface
	class IRibbonPointSource {
	public:
		/// @brief Point Source Interfaceを破棄
		virtual ~IRibbonPointSource() = default;

		/// @brief 制御点の経過時間と寿命を更新
		/// @param deltaTime 前フレームからの経過時間
		virtual void Update(float deltaTime) = 0;

		/// @brief 追跡対象Transformを更新
		/// @param transform 最新Transform
		virtual void SetTransform(const Transform3D& transform) = 0;

		/// @brief 新規Point生成を停止
		/// @param mode 停止方式
		virtual void Stop(RibbonStopMode mode) = 0;

		/// @brief 保持しているPointをすべて破棄
		virtual void Clear() = 0;

		/// @brief Rendererへ渡すPoint列を取得
		/// @return 古い順に並んだPoint列
		virtual const std::vector<RibbonPoint>& GetPoints() const = 0;
	};

} // namespace MadoEngine::Ribbon
