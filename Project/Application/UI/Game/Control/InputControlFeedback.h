#pragma once
#include "Render/Object/ObjectHandle.h"

namespace UI::Game {

	/// @brief Editor配置の操作案内に対する共通の押下演出
	class InputControlFeedback final {
	public:
		/// @brief ボタンGroupの押下演出と子Spriteの前面表示を更新
		/// @param deltaTime 停止中も進行する実時間
		/// @param actionName 押下状態を参照する入力名
		/// @param buttonGroup ボタンGroupのハンドル
		void Update(float deltaTime, const char* actionName, MadoEngine::UIGroupHandle buttonGroup);

		/// @brief 変更したScaleを基準値へ戻して演出状態を初期化
		void Reset();

	private:
		MadoEngine::UIGroupHandle buttonGroup_{};
		float pressAmount_ = 0.0f;
	};
}
